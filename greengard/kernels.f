      subroutine cart2polar(zat,r,theta,phi)
      implicit real *8 (a-h,o-z)
      real *8 zat(3)
      complex *16 ephi,eye
      data eye/(0.0d0,1.0d0)/
      r= sqrt(zat(1)**2+zat(2)**2+zat(3)**2)
      proj = sqrt(zat(1)**2+zat(2)**2)
      theta = datan2(proj,zat(3))
      if( abs(zat(1)) .eq. 0 .and. abs(zat(2)) .eq. 0 ) then
      phi = 0
      else
      phi = datan2(zat(2),zat(1))
      endif
      return
      end
c**********************************************************************
      subroutine P2P(ibox,target,pot,fld,jbox,source,charge,wavek) 
c**********************************************************************
c     This subroutine calculates the potential POT and field FLD
c     at the target point TARGET, due to a charge at 
c     SOURCE. The scaling is that required of the delta function
c     response: i.e.,
c     
c              	pot = exp(i*k*r)/r
c		fld = -grad(pot)
c---------------------------------------------------------------------
c     INPUT:
c     source    : location of the source 
c     charge    : charge strength
c     target    : location of the target
c     wavek        : helmholtz parameter
c---------------------------------------------------------------------
c     OUTPUT:
c     pot       : calculated potential
c     fld       : calculated gradient
c---------------------------------------------------------------------
      implicit none
      integer i,j,ibox(20),jbox(20)
      real *8 dx,dy,dz,R2,R,target(3,1000000),source(3,1000000)
      complex *16 i1,wavek,coef1,coef2
      complex *16 charge(1000000),pot(1000000),fld(3,1000000)
      data i1/(0.0d0,1.0d0)/
      do i=ibox(14),ibox(14)+ibox(15)-1
         do j=jbox(14),jbox(14)+jbox(15)-1
            dx=target(1,i)-source(1,j)
            dy=target(2,i)-source(2,j)
            dz=target(3,i)-source(3,j)
            R2=dx*dx+dy*dy+dz*dz
            if(R2.eq.0) cycle
            R=sqrt(R2)
            coef1=charge(j)*cdexp(i1*wavek*R)/R
            coef2=(1-i1*wavek*R)*coef1/R2
            pot(i)=pot(i)+coef1
            fld(1,i)=fld(1,i)+coef2*dx
            fld(2,i)=fld(2,i)+coef2*dy
            fld(3,i)=fld(3,i)+coef2*dz
         enddo
      enddo
      return
      end
C***********************************************************************
      subroutine P2M
     1     (ier,wavek,rscale,source,charge,ns,center,
     1     nterms,nterms1,lwfjs,mpole,wlege,nlege)
C***********************************************************************
C
C     Constructs multipole (h) expansion about CENTER due to NS sources 
C     located at SOURCES(3,*).
C
c-----------------------------------------------------------------------
C     INPUT:
c
C     wavek              : Helmholtz parameter 
C     scale           : the scaling factor.
C     sources         : coordinates of sources
C     charge          : source strengths
C     ns              : number of sources
C     center          : epxansion center
C     nterms          : order of multipole expansion
C     nterms1         : order of truncated expansion
c     wlege  :    precomputed array of scaling coeffs for Pnm
c     nlege  :    dimension parameter for wlege
C
c-----------------------------------------------------------------------
C     OUTPUT:
C
c     ier             : error return code
c     mpole           : coeffs of the h-expansion
c-----------------------------------------------------------------------
      implicit none
      integer i,j,m,l,n,ns,nterms,nterms1,nlege,ifder,lwfjs,jer,ntop,ier
      integer iscale(0:lwfjs)
      real *8 r,theta,phi,ctheta,stheta,cphi,sphi,wlege,rscale,dtmp
      real *8 thresh
      real *8 center(3),source(3,ns),dX(3)
      real *8 pp(0:nterms,0:nterms)
      real *8 ppd(0:nterms,0:nterms)
      complex *16 charge(ns),i1,wavek,z,ztmp,ephi1,ephi1inv
      complex *16 ephi(-nterms-1:nterms+1)
      complex *16 fjs(0:lwfjs),fjder(0:lwfjs)
      complex *16 mpole(0:nterms,-nterms:nterms)
      complex *16 mtemp(0:nterms,-nterms:nterms)
      data i1/(0.0d0,1.0d0)/
      data thresh/1.0d-15/
      ier=0
      do l = 0,nterms
         do m=-l,l
            mtemp(l,m) = 0
         enddo
      enddo
      do i = 1, ns
         dX(1)=source(1,i)-center(1)
         dX(2)=source(2,i)-center(2)
         dX(3)=source(3,i)-center(3)
         call cart2polar(dX,r,theta,phi)
         ctheta=dcos(theta)
         stheta=dsin(theta)
         cphi=dcos(phi)
         sphi=dsin(phi)
         ephi1=dcmplx(cphi,sphi)
         ephi(0)=1.0d0
         ephi(1)=ephi1
         ephi(-1)=dconjg(ephi1)
         do j=2,nterms+1
            ephi(j)=ephi(j-1)*ephi1
            ephi(-j)=ephi(-j+1)*ephi(-1)
         enddo
         call ylgndrfw(nterms1,ctheta,pp,wlege,nlege)
         ifder=0
         z=wavek*r
         call jfuns3d(jer,nterms1,z,rscale,fjs,ifder,fjder,
     1	      lwfjs,iscale,ntop)
         do n = 0,nterms1
            fjs(n) = fjs(n)*charge(i)
         enddo
         mtemp(0,0)= mtemp(0,0) + fjs(0)
         do n=1,nterms1
            dtmp=pp(n,0)
            mtemp(n,0)= mtemp(n,0) + dtmp*fjs(n)
            do m=1,n
               ztmp=pp(n,m)*fjs(n)
               mtemp(n, m)= mtemp(n, m) + ztmp*dconjg(ephi(m))
               mtemp(n,-m)= mtemp(n,-m) + ztmp*dconjg(ephi(-m))
            enddo
         enddo
      enddo
      do l = 0,nterms
         do m=-l,l
            mpole(l,m) = mpole(l,m)+mtemp(l,m)*i1*wavek
         enddo
      enddo
      return
      end
c**********************************************************************
      subroutine L2P(wavek,rscale,center,locexp,nterms,
     1     nterms1,lwfjs,target,nt,pot,fld,wlege,nlege,ier)
c**********************************************************************
c     This subroutine evaluates a j-expansion centered at CENTER
c     at the target point TARGET. 
c
c     pot =  sum sum  locexp(n,m) j_n(k r) Y_nm(theta,phi)
c             n   m
c---------------------------------------------------------------------
c     INPUT:
c     wavek      : the Helmholtz coefficient
c     rscale     : scaling parameter used in forming expansion
c     center     : coordinates of the expansion center
c     locexp     : coeffs of the j-expansion
c     nterms     : order of the h-expansion
c     nterms1    : order of the truncated expansion
c     target(3)   : target vector
c     nt         : number of targets
c     wlege  :    precomputed array of scaling coeffs for Pnm
c     nlege  :    dimension parameter for wlege
c---------------------------------------------------------------------
c     OUTPUT:
c     ier        : error return code
c     pot        : potential at target (if requested)
c     fld(3)     : gradient at target (if requested)
c     NOTE: Parameter lwfjs is set to nterms+1000
c           Should be sufficient for any Helmholtz parameter
c---------------------------------------------------------------------
      implicit none
      integer i,j,m,n,nt,ier,jer,nterms,nterms1,nlege,ntop,lwfjs
      integer iscale(0:lwfjs)
      real *8 r,rx,ry,rz,theta,thetax,thetay,thetaz,rscale
      real *8 phi,phix,phiy,phiz,ctheta,stheta,cphi,sphi,wlege
      real *8 center(3),target(3,1),dX(3)
      real *8 pp(0:nterms,0:nterms)
      real *8 ppd(0:nterms,0:nterms)
      complex *16 wavek,pot(1),fld(3,1),ephi1,ephi1inv
      complex *16 locexp(0:nterms,-nterms:nterms)
      complex *16 ephi(-nterms-1:nterms+1)
      complex *16 fjsuse,fjs(0:lwfjs),fjder(0:lwfjs)
      complex *16 eye,ur,utheta,uphi,ztmp,z
      complex *16 ztmp1,ztmp2,ztmp3,ztmpsum
      complex *16 ux,uy,uz
      data eye/(0.0d0,1.0d0)/
      ier=0
      do i=1,nt
         dX(1)=target(1,i)-center(1)
         dX(2)=target(2,i)-center(2)
         dX(3)=target(3,i)-center(3)
         call cart2polar(dX,r,theta,phi)
         ctheta = dcos(theta)
         stheta=sqrt(1-ctheta*ctheta)
         cphi = dcos(phi)
         sphi = dsin(phi)
         ephi1 = dcmplx(cphi,sphi)
         ephi(0)=1.0d0
         ephi(1)=ephi1
         ephi(-1)=dconjg(ephi1)
         do j=2,nterms+1
            ephi(j)=ephi(j-1)*ephi1
            ephi(-j)=ephi(-j+1)*ephi(-1)
         enddo
         rx = stheta*cphi
         thetax = ctheta*cphi
         phix = -sphi
         ry = stheta*sphi
         thetay = ctheta*sphi
         phiy = cphi
         rz = ctheta
         thetaz = -stheta
         phiz = 0.0d0
         call ylgndr2sfw(nterms1,ctheta,pp,ppd,wlege,nlege)
         z=wavek*r
         call jfuns3d(jer,nterms1,z,rscale,fjs,1,fjder,
     1	      lwfjs,iscale,ntop)
         if (jer.ne.0) then
            ier=8
            return
         endif
         pot(i)=pot(i)+locexp(0,0)*fjs(0)
         do j=0,nterms1
            fjder(j)=fjder(j)*wavek
         enddo
         ur = locexp(0,0)*fjder(0)
         utheta = 0.0d0
         uphi = 0.0d0
         do n=1,nterms1
            pot(i)=pot(i)+locexp(n,0)*fjs(n)*pp(n,0)
            ur = ur + fjder(n)*pp(n,0)*locexp(n,0)
            fjsuse = fjs(n+1)*rscale + fjs(n-1)/rscale
            fjsuse = wavek*fjsuse/(2*n+1.0d0)
            utheta = utheta -locexp(n,0)*fjsuse*ppd(n,0)*stheta
            do m=1,n
               ztmp1=fjs(n)*pp(n,m)*stheta
               ztmp2 = locexp(n,m)*ephi(m) 
               ztmp3 = locexp(n,-m)*ephi(-m)
               ztmpsum = ztmp2+ztmp3
               pot(i)=pot(i)+ztmp1*ztmpsum
               ur = ur + fjder(n)*pp(n,m)*stheta*ztmpsum
               utheta = utheta -ztmpsum*fjsuse*ppd(n,m)
               ztmpsum = eye*m*(ztmp2 - ztmp3)
               uphi = uphi + fjsuse*pp(n,m)*ztmpsum
            enddo
         enddo
         ux = ur*rx + utheta*thetax + uphi*phix
         uy = ur*ry + utheta*thetay + uphi*phiy
         uz = ur*rz + utheta*thetaz + uphi*phiz
         fld(1,i) = fld(1,i)-ux
         fld(2,i) = fld(2,i)-uy
         fld(3,i) = fld(3,i)-uz
      enddo
      return
      end


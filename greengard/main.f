      program main
      use omp_lib, only : omp_get_wtime
      implicit none
      integer nsource,ntarget,i,iprec
      integer ibox(20),jbox(20)
      real *8 pi,tic,toc,pdiff,pnorm,fdiff,fnorm
      real *8 source(3,1000000)
      complex *16 charge(1000000)
      complex *16 pot(1000000)
      complex *16 fld(3,1000000)
      complex *16 pot2(1000000)
      complex *16 fld2(3,1000000)
      complex *16 wavek,ima/(0.0d0,1.0d0)/
      pi=4*atan(1.0d0)
      nsource= 100000
      wavek = 1.0d0 + ima*0.1d0
      call random_number(source)
      print*,'N      =',nsource
      iprec=4
      print*,'iprec  =',iprec
      do i=1,nsource
         charge(i)=source(1,i)+ima*source(2,i)
      enddo
c     FMM
c$    tic=omp_get_wtime()
      call fmm(iprec, wavek,
     1     nsource,source,charge,
     1     pot,fld)
c$    toc=omp_get_wtime()
      print*,'FMM    =',toc-tic
c     Direct
      ntarget = min(nsource,100)
      do i=1,ntarget
         pot2(i) = 0
         fld2(1,i) = 0
         fld2(2,i) = 0
         fld2(3,i) = 0
      enddo
      ibox(14) = 1
      ibox(15) = ntarget
      jbox(14) = 1
      jbox(15) = nsource
c$    tic=omp_get_wtime()
      call P2P(ibox,source,pot2,fld2,jbox,source,charge,wavek)
c$    toc=omp_get_wtime()
      print*,'Direct =',toc-tic
      pdiff = 0
      pnorm = 0
      fdiff = 0
      fnorm = 0
      do i=1,ntarget
         pdiff = pdiff+abs(pot(i)-pot2(i))**2
         pnorm = pnorm+abs(pot2(i))**2
         fdiff = fdiff+abs(fld(1,i)-fld2(1,i))**2
     1        +abs(fld(2,i)-fld2(2,i))**2
     1        +abs(fld(3,i)-fld2(3,i))**2
         fnorm = fnorm+abs(fld2(1,i))**2
     1        +abs(fld2(2,i))**2
     1        +abs(fld2(3,i))**2
      enddo
      print*,'Err pot=',sqrt(pdiff/pnorm)
      print*,'Err acc=',sqrt(fdiff/fnorm)
      stop
      end

import ctypes, numpy
from matplotlib import pyplot, cm
from mpl_toolkits.mplot3d import Axes3D
lib = ctypes.CDLL('./libstep01.so')
lib.navierstokes.argtypes = [numpy.ctypeslib.ndpointer(dtype=numpy.uintp, ndim=1, flags='C'),
                             numpy.ctypeslib.ndpointer(dtype=numpy.uintp, ndim=1, flags='C'),
                             numpy.ctypeslib.ndpointer(dtype=numpy.uintp, ndim=1, flags='C'),
                             ctypes.c_int, ctypes.c_int, ctypes.c_double, ctypes.c_double,
                             ctypes.c_int, ctypes.c_double, ctypes.c_double, ctypes.c_double]

nx = 401
ny = 401
dx = 2./(nx-1)
dy = 2./(ny-1)
nt = 3
nit = 500
rho = 1
nu = .01
dt = .0001
x = numpy.linspace(0,2,nx)
y = numpy.linspace(0,2,ny)
X, Y = numpy.meshgrid(x,y)
u = numpy.zeros((ny,nx))
v = numpy.zeros((ny,nx))
p = numpy.zeros((ny,nx))
upp = (u.__array_interface__['data'][0] + numpy.arange(u.shape[0])*u.strides[0]).astype(numpy.uintp)
vpp = (v.__array_interface__['data'][0] + numpy.arange(v.shape[0])*v.strides[0]).astype(numpy.uintp)
ppp = (p.__array_interface__['data'][0] + numpy.arange(p.shape[0])*p.strides[0]).astype(numpy.uintp)

fig = pyplot.figure(figsize=(11,7), dpi=100)

for n in range(nt):
    print n
    lib.navierstokes(upp, vpp, ppp, nx, ny, dx, dy, nit, rho, nu, dt)
    pyplot.contourf(X[::20,::20], Y[::20,::20], p[::20,::20], alpha=0.5)
    pyplot.colorbar()
    pyplot.quiver(X[::20,::20], Y[::20,::20], u[::20,::20], v[::20,::20])
    pyplot.xlabel('X')
    pyplot.ylabel('Y');
    pyplot.pause(0.05)
    pyplot.clf()
pyplot.show()
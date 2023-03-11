from PIL import Image
import numpy as np
from mpi4py import MPI

comm = MPI.COMM_WORLD
rank = comm.Get_rank()
size = comm.Get_size()

w, h, zoom = 1920,1080,1

cX, cY = -0.7, 0.27015
moveX, moveY = 0.0, 0.0
maxIter = 255

if rank != 0:
    # Allocate a new array for each process to store the data for pixels
    local_w_start = (rank - 1) * (w // (size - 1)) + min(rank - 1, w % (size - 1))
    local_w_end = rank * (w // (size - 1)) + min(rank, w % (size - 1)) - 1
    local_h_start = (rank - 1) * (h // (size - 1)) + min(rank - 1, h % (size - 1))
    local_h_end = rank * (h // (size - 1)) + min(rank, h % (size - 1)) - 1
    pix = np.empty((local_w_end - local_w_start, local_h_end - local_h_start, 3), dtype=np.uint8)
    for x in range(local_w_start, local_w_end):
        for y in range(local_h_start, local_h_end):
            zx = 1.5*(x - w/2)/(0.5*zoom*w) + moveX
            zy = 1.0*(y - h/2)/(0.5*zoom*h) + moveY
            i = maxIter
            while zx*zx + zy*zy < 4 and i > 1:
                tmp = zx*zx - zy*zy + cX
                zy,zx = 2.0*zx*zy + cY, tmp
                i -= 1
            pix[x,y] = (i << 21) + (i << 10) + i*8
    comm.send(local_w_start, dest=0, tag=0)
    comm.send(local_w_end, dest=0, tag=1)
    comm.send(local_h_start, dest=0, tag=2)
    comm.send(local_h_end, dest=0, tag=3)
    comm.Send([pix, MPI.INT], dest=0, tag=4)
else:
    bitmap = Image.new("RGB", (w, h), "white")
    for i in range(1, size):
        final_image = bitmap.load()
        w_start = comm.recv(source=i, tag=0)
        w_end = comm.recv(source=i, tag=1)
        h_start = comm.recv(source=i, tag=2)
        h_end = comm.recv(source=i, tag=3)
        pix = np.empty((w_end - w_start, h_end - h_start, 3), dtype=np.uint8)
        comm.Recv([pix, MPI.INT], source=i, tag=4)
        final_image[w_start:w_end, h_start:h_end] = pix
    final_image.save("output.jpeg")

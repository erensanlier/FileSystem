

Test Results:

input_8_600_5_5_0:

Contigious:                             Linked:

Create Errors: 212                  Create Errors: 396
Extend Errors: 203                 Extend Errors: 362
Average Time: 1,507898 s     Average Time:  0,2210184 s

input_1024_200_5_9_9:

Contigious:                              Linked:
Create Errors: 162                   Create Errors: 163
Extend Errors: 1193                Extend Errors: 1190
Average Time: 0,01363116 s  Average Time:  0,874824 s

input_1024_200_9_0_0:

Contigious:                             Linked:
Create Errors: 80                    Create Errors: 79
Extend Errors: 0                     Extend Errors: 0
Average Time: 0,5048316 s   Average Time: 1,25941 s

input_1024_200_9_0_9:

Contigious:                             Linked:
Create Errors: 71                    Create Errors: 72
Extend Errors: 0                      Extend Errors: 0
Average Time: 0,04245018 s  Average Time: 0,875126 s

input_2048_600_5_5_0:

Contigious:                             Linked:
Create Errors: 208                  Create Errors: 213
Extend Errors: 184                 Extend Errors: 178
Average Time: 1,41124 s       Average time: 0,8333756 s

• With test instances having a block size of 1024, in which cases (inputs) contiguous
allocation has a shorter average operation time? Why? What are the dominating
operations in these cases? In which linked is better, why?(10 pt.)

Contigious allocation had significantally better performance when block size was 1024 bytes. Comparing those input files, "input_1024_200_9_0_0" had much shorter average operation time. That is because, in that input file, there is only access operation. Looking to contigious files, finding a byte offset from the start block of a file takes constant time. Also in "input_1024_200_9_0_0", in which access is dominated, linked allocation has better performance compared to other. It is because other operations in linked allocation take O(n^2) time. It is because in linked allocation, we have to find a new block for extend and create operations (which takes (O(n^2)). Access in general was the least costly operation.


• Comparing the difference between the creation rejection ratios with block size 2048 and
8, what can you conclude? How did dealing with smaller block sizes affect the FAT
memory utilization? (5 pt.)

It was disasterous to use FAT for smaller block size. We had nearly twice as much rejections. Comparing 8 byte and 2048 byte block sizes, we have to look what percentage does our FAT table allocates space on disk. For 2048 bytes, it's only %0.2 but for 8 bytes, its %50. That means in 8 bytes of block sizes, we had to reserve half of our space just for FAT table.

• FAT is a popular way to implement linked allocation strategy. This is because it permits
faster access compared to the case where the pointer to the next block is stored as a
part of the concerned block. Explain why this provides better space utilization. (5 pt.)

My first implementation idea was to reduce block size by 4 bytes, so that every block could hold the pointer to the next one. At that point I realized that I am implementing the linked allocation but without a FAT. FAT helps utilization, since if we store our pointers in the blocks, when we delete files we have to take care of that pointer. Furthermore, if we mess up with a block, some file content cannot be accessed. That's why it is a great idea to partition the dist for FAT table content.

• If you have extra memory available of a size equal to the size of the DT, how can this
improve the performance of your defragmentation?(3 pt.)

After every shift process, my program finds the next file in the memory by going through all entries in the table. This takes O(n) time. I would use the extra space to keep a pointer to the next file for each file in the memory. This would indeed save me from O(n^2) time (it doesn't change much though if our files are long). 


• How much, at minimum, extra memory do you need to guarantee reduction in the
number of rejected extensions in the case of contiguous allocations? ( 3 pt.)

Assuming we have passed previous control points, we come to a position that we have a file, and no available space to move the file to the end of the directory, defrag and make the extension. We would need (remaining_space - file_size) more space, so that we could create space to put our file to the end, defrag and make extension.

# main topics

## algorithm 

1. parent wait children with select read and write ( for children for whom data available to send )
2. write or read from all selected fd then wait again ( write is more preferable then read )
3. set fd to wait and select again

## rules

1.  when read == 0, then data is runout, first child reached end_of_file and exit. Then exclude this fd from wait_for_read and continue to work.
If exit not first in order to exit child, then error. 
2. child exit when there is no more for read and buff flushed to parent. 
3. data should treated as little as one byte can be written each read or write iteration. 
4. kill first child == end of sending
5. data stored in parent as queue 
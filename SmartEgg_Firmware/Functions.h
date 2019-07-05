#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/* Maps a float to to a range */
#define mapf(val, in_min, in_max, out_min, out_max) \
  ((float) val - (float) in_min)\
  * ((float) out_max - (float) out_min)\
  / ((float) in_max - (float) in_min)\
  + (float) out_min

#endif /* FUNCTIONS_H */

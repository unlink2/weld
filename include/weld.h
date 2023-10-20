#ifndef weldH__
#define weldH__

struct weld_config {
  _Bool verbose;
};

int weld_main(struct weld_config *cfg);

#endif 

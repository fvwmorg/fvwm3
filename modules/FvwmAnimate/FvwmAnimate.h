#ifndef _ANIMATE_H_
#define _ANIMATE_H_

/* Animation granularity. How many iterations use for the animation.  */
#define ANIM_ITERATIONS 12

/* delay for each iteration of the animation in ms */
#define ANIM_DELAY      1

/* delay for each iteration of the close animation in ms */
#define ANIM_DELAY2     20

/* distance to spin frame around, 1.0 is one revolution.
   with large values you should up ANIM_ITERATIONS as well */
#define ANIM_TWIST      0.5

/* default line width */
#define ANIM_WIDTH      0

/* default time */
#define ANIM_TIME      0

struct ASAnimate {
  char *color;
  int iterations;
  int delay;
  float twist;
  int width;
  void (*resize)(int, int, int, int, int, int, int, int);
  clock_t time;
};

extern struct ASAnimate Animate;

#endif /* _ANIMATE_H_ */

/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

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

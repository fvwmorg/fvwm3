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

#include <libs/fvwmlib.h>
#include <fvwm/fvwm.h>
#include <libs/vpacket.h>

struct list
{
  unsigned long id;
  int frame_height;
  int frame_width;
  int base_width;
  int base_height;
  int width_inc;
  int height_inc;
  int frame_x;
  int frame_y;
  int title_height;
  int boundary_width;
  window_flags flags;
  unsigned long gravity;
  struct list *next;
};

/*************************************************************************
 *
 * Subroutine Prototypes
 *
 *************************************************************************/
void Loop(int *fd);
struct list *find_window(unsigned long id);
void add_window(unsigned long new_win, unsigned long *body);
void DeadPipe(int nonsense) __attribute__((noreturn));
void process_message(unsigned long type,unsigned long *body);
void do_save(void);
void list_new_page(unsigned long *body);


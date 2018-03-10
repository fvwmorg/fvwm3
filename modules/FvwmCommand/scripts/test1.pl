#! xPERLx
# arg1  t to invoke FvwmTalk
#       td  to kill FvwmTalk
#       none to move windows

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see: <http://www.gnu.org/licenses/>

use FvwmCommand;


if( $ARGV[0] eq 't' ) {
	Desk ( 0,1);
	GotoPage (1, 1);
	Module (FvwmTalk);
}elsif( $ARGV[0] eq 'td' ) {
	KillModule (FvwmTalk);
}else {
	Move ;
}

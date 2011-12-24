/*
  memqueue
  (C) 2011  Hasan Alayli <halayli@gmail.com>

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  poller.h
*/

#ifndef _LTHREAD_POLLER_H_
#define _LTHREAD_POLLER_H_ 
inline void register_rd_interest(int fd);
inline void register_wr_interest(int fd);
inline void clear_interest(int fd);
int create_poller(void);
inline int poll_events(struct timespec t);

#if defined(__FreeBSD__) || defined(__APPLE__)
inline int get_event(struct kevent *ev);
inline int get_fd(struct kevent *ev);
inline void *get_data(struct kevent *ev);
inline int is_eof(struct kevent *ev);
#else
int get_event(struct epoll_event *ev);
int get_fd(struct epoll_event *ev);
void *get_data(struct epoll_event *ev);
inline int is_eof(struct epoll_event *ev);
#endif

#endif

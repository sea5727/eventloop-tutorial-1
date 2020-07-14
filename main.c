#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>
#include <time.h>
#define EPOLL_SIZE 1024

typedef struct _my_packets
{
    int a;
    int b;
    char c;
    char d;
    char e[4];
    int f;
}_my_packets;


typedef void (*my_timer_cb)(void* handle);


int timer_count = 0;

int _my_loop_fd;


int _my_timer_fd;
my_timer_cb _my_timer_cb;
void *_my_pdata;

int 
my_loop_init()
{
    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if(epoll_fd < 0) return -1;

    _my_loop_fd = epoll_fd;

    //this..
    //backend_fd = fd;
    //inotify_fd = -1;
    // inotify_watchers = NULL;
    //nfds
    //watchers
    //nwatchers


    return 0;
}
int 
my_loop_run()
{
    struct epoll_event events[1024];
    while(1)
    {
        memset(&events, 0, sizeof(events));
        int wait_result = epoll_wait(_my_loop_fd, events, 1024, 500);
        if(wait_result < 0) 
        {
            printf("error : %d\n", wait_result);
            continue;
        }
        
        for(int i = 0 ; i < wait_result ; i++)
        {
            
            uint64_t res;
            read(events[i].data.fd, &res, sizeof(uint64_t));
            printf("receved fd : %d, events : %d, res:%d \n", events[i].data.fd, events[i].events, res);
            if(events[i].data.fd == _my_timer_fd)
            {
                timer_count += 1;
                printf("timer!! count : %d\n", timer_count);
                if(timer_count == 10) 
                {
                    close(events[i].data.fd);
                    struct epoll_event ev;
                    ev.events = EPOLLOUT;
                    epoll_ctl(_my_loop_fd, EPOLL_CTL_DEL, _my_timer_fd, &ev);
                    break;
                }
                
                _my_timer_cb(_my_pdata);
            }
        }

    }
    return 0;
}

int 
my_timer_init(struct itimerspec ts)
{
    _my_timer_fd = timerfd_create(0, EPOLL_CLOEXEC | EPOLL_NONBLOCK);
    if(_my_timer_fd < 0 ) return -1;


	if (timerfd_settime(_my_timer_fd, 0, &ts, NULL) < 0) 
    {
		close(_my_timer_fd);
		return -1;
	}

    return 0;
}



int
my_timer_run(int epoll_fd, int timer_fd, my_timer_cb cb, void *pdata)
{
    struct epoll_event events;
    events.data.fd = timer_fd;
    events.events = EPOLLIN;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &events);

    _my_timer_cb = cb;
    _my_pdata = pdata;
}



void
callback_function(void *handle)
{
    _my_packets *pdata = (_my_packets *)handle;
    printf("test callback.. \n");
    printf("a:%d, b:%d, c:%c, d:%c..\n", pdata->a, pdata->b, pdata->c, pdata->d);
}

int main(int argc, char *argv[])
{
    int result = -1;
    printf("Hello world!!\n");
    my_loop_init();

    struct itimerspec ts;
	ts.it_interval.tv_sec = 1;          // interval(sec)
	ts.it_interval.tv_nsec = 0;         // interval(nano_sec)
	ts.it_value.tv_sec = 0;             // timeout(sec)
	ts.it_value.tv_nsec = 500000000;    // timeout(nano_sec) : 900ms

    result = my_timer_init(ts);
    
    _my_packets my_packets;
    my_packets.a = 1000;
    my_packets.b = 100;
    my_packets.c = 'a';
    my_packets.d = 'e';

    my_timer_run(_my_loop_fd, _my_timer_fd, callback_function, (void *)&my_packets);
    printf("my_timer_init result : %d\n", result);
    my_loop_run();


    return 0;
}
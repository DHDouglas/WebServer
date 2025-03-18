#include "epoller.h"

#include <sys/epoll.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <unistd.h>

#include "logger.h"
#include "eventloop.h"

Epoller::Epoller(EventLoop* loop)
    : owner_loop_(loop), 
    epoll_fd_(epoll_create1(EPOLL_CLOEXEC)),
    events_(kInitEventListSize)
{
    if (epoll_fd_ == -1) {
        LOG_SYSFATAL << "EPollPoller::EPollPoller";
    }

}

Epoller::~Epoller() {
    ::close(epoll_fd_); 
}

int Epoller::getEpollFd() const {
    return epoll_fd_; 
}

Timestamp Epoller::poll(int timeout_ms, ChannelList* activeChannels) {
    int nfds = epoll_wait(epoll_fd_, events_.data(), static_cast<int>(events_.size()),  timeout_ms); 
    Timestamp now(Timestamp::now());
    int savedErrno = errno; 
    if (nfds > 0) {
        fillActiveChannels(nfds, activeChannels); 
        if (static_cast<size_t>(nfds) == events_.size()) {
            events_.resize(events_.size() * 2);   // 根据I/O繁忙程度扩容.
        }
    } else if (nfds == 0) {
        LOG_TRACE << "nothing happend"; 
    } else {
        if (savedErrno != EINTR) {
            errno = savedErrno; 
            LOG_SYSERR << "EPoller::poll()"; 
        }
    }
    return now; 
}

void Epoller::fillActiveChannels(int nfds, ChannelList* activeChannels) const {
    // 根据nfds, 设置对应channel中的事件
    for (int i = 0; i < nfds; ++i) {
        // struct epoll_event结构中的`.data`结构里的ptr字段, 可用于为fd绑定用户自定义的关联数据.
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);  
        channel->setRevents(events_[i].events); 
        activeChannels->push_back(channel); 
    }
}

bool Epoller::hasChannel(Channel* channel) {
    assertInLoopThread();
    const auto& it = channels_.find(channel->getFd());
    return it != channels_.end() && it->second == channel;  
}


void Epoller::updateChannel(Channel* channel) {
    assertInLoopThread();
    using State = Channel::StateInEpoll; 
    int fd = channel->getFd();
    State state = channel->getState(); 
    int events = channel->getEvents(); 
    LOG_TRACE << "fd = " << fd
              << " events = " << events
              << " state = " << static_cast<int>(state);

    if (state == State::NOT_EXIST) {
        assert(channels_.find(fd) == channels_.end()); 
        channels_[fd] = channel;
    } else if (state == State::DETACHED) {
        assert(channels_.find(fd) != channels_.end()); 
        assert(channels_[fd] == channel); 
    } else {  // State::LISTENING
        assert(channels_.find(fd) != channels_.end());
        assert(channels_[fd] == channel); 
        assert(state == State::LISTENING); 
    }

    if (channel->isNoneEvent()) {
        if (state == State::LISTENING) {
            epollUpdate(EPOLL_CTL_DEL, channel);  
        }
        channel->setState(State::DETACHED);  // 未指定监听事件, 不注册到epoll. 
    } else {
        if (state == State::LISTENING) {
            epollUpdate(EPOLL_CTL_MOD, channel); 
        } else {
            epollUpdate(EPOLL_CTL_ADD, channel); // NOT_EXISTS或DETACHE, 都重新注册到epoll.
        }
        channel->setState(State::LISTENING);
    }
}


void Epoller::removeChannel(Channel* channel) {
    assertInLoopThread();  
    int fd = channel->getFd(); 
    using State = Channel::StateInEpoll; 
    State state = channel->getState(); 
    LOG_TRACE << "fd = " << fd << " state = " << static_cast<int>(state);
    assert(channels_.find(fd) != channels_.end()); 
    assert(channels_[fd] == channel); 
    assert(channel->isNoneEvent()); 
    assert(state == State::LISTENING || state == State::DETACHED);  

    size_t rec = channels_.erase(fd);
    assert(rec == 1);
    (void)rec;

    if (state == State::LISTENING) {
        epollUpdate(EPOLL_CTL_DEL, channel); 
    }
    channel->setState(State::NOT_EXIST);  
}


void Epoller::epollUpdate(int epoll_op, Channel* channel) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev)); 
    int fd = channel->getFd();
    ev.data.fd = fd;
    ev.data.ptr = channel;
    ev.events = channel->getEvents();  
    if (epoll_ctl(epoll_fd_, epoll_op, fd, &ev) < 0) {
        switch(epoll_op) {
            case EPOLL_CTL_ADD: {
                LOG_SYSERR << "epoll_ctl(EPOLL_CTL_ADD)" << " fd =" << fd;
            } break;
            case EPOLL_CTL_MOD: {
                LOG_SYSERR << "epoll_ctl(EPOLL_CTL_MOD)" << " fd =" << fd;
            } break; 
            case EPOLL_CTL_DEL: {
                LOG_SYSFATAL << "epoll_ctl(EPOLL_CTL_DEL)" << " fd =" << fd;
            } break; 
            default: {
                LOG_SYSFATAL << "epoll_ctl(Unknwon op)" << " fd =" << fd;
            }
        }
    }
}

void Epoller::assertInLoopThread() const {
    owner_loop_->assertInLoopThread(); 
}
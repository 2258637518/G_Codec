#ifndef SAVEQUEUE_H
#define SAVEQUEUE_H
#include<queue>
#include<mutex>
#include<condition_variable>
extern "C"
{
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavcodec/packet.h>
}
template <typename T>
class SaveQueue
{
private:
    std::queue<T>q;
    std::mutex mtx;
    std::condition_variable cond_not_empty;
    std::condition_variable cond_not_full;
    bool abort_request = false;

public:
    SaveQueue() = default;
    size_t max_size;
    void set_size(size_t size)
    {
        max_size = size;
    }
    void abort()
    {
        std::lock_guard<std::mutex> lock(mtx);
        abort_request = true;
        cond_not_empty.notify_all();
        cond_not_full.notify_all();
    }
    void push(T t)
    {
        std::unique_lock<std::mutex>unique(mtx);
        cond_not_full.wait(unique, [this]() {
            return q.size() < max_size || abort_request;
        });
        if(abort_request){
            free_av_data(t);
            return;
        }
        q.push(t);
        cond_not_empty.notify_one();
    }
    T pop()
    {
        std::unique_lock<std::mutex> unique(mtx);
        cond_not_empty.wait(unique,[this](){
            return !q.empty() || abort_request;
        });
        if(abort_request || q.empty())return nullptr;
        T t=q.front();
        q.pop();
        cond_not_full.notify_one();
        return t;
    }
    void clear()
    {
        std::lock_guard<std::mutex>lock(mtx);
        while(!q.empty())
        {
            T t = q.front();
            free_av_data(t);
            q.pop();
        }
        cond_not_empty.notify_all();
        cond_not_full.notify_all();
    }
    int size()
    {
        std::lock_guard<std::mutex> lock(mtx);
        return q.size();
    }

    // 1. 重载版本：专门安全删除 AVPacket
    inline void free_av_data(AVPacket*& pkt) {
        if (pkt) {
            av_packet_free(&pkt); // 安全释放 AVPacket，并将外部指针置为 nullptr
        }
    }

    // 2. 重载版本：专门安全删除 AVFrame
    inline void free_av_data(AVFrame*& frame) {
        if (frame) {
            av_frame_free(&frame); // 安全释放 AVFrame，并将外部指针置为 nullptr
        }
    }
};

#endif // SAVEQUEUE_H

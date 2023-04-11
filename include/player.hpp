#if __has_include(<Arduino.h>)
#include <Arduino.h>
#else
#include <stddef.h>
#include <string.h>
#endif
// info used for custom voice functions
typedef struct voice_function_info {
    void* buffer;
    size_t frame_count;
    unsigned int channel_count;
    unsigned int bit_depth;
    unsigned int sample_max;
} voice_function_info_t;
// custom voice function
typedef void (*voice_function_t)(const voice_function_info_t& info, void* state);
// the handle to refer to a playing voice
typedef void* voice_handle_t;
// called when the sound output should be disabled
typedef void (*player_on_sound_disable_callback)(void* state);
// called when the sound output should be enabled
typedef void (*player_on_sound_enable_callback)(void* state);
// called when there's sound data to send to the output
typedef void (*player_on_flush_callback)(const void* buffer, size_t buffer_size, void* state);
// called to read a byte off a stream
typedef int (*player_on_read_stream_callback)(void* state);
// called to seek a stream
typedef void (*player_on_seek_stream_callback)(unsigned long long pos, void* state);
// represents a polyphonic player capable of playing wavs or various waveforms
class player final {
    voice_handle_t m_first;
    void* m_buffer;
    size_t m_frame_count;
    unsigned int m_sample_rate;
    unsigned int m_channel_count;
    unsigned int m_bit_depth;
    unsigned int m_sample_max;
    bool m_auto_disable;
    bool m_sound_enabled;
    player_on_sound_disable_callback m_on_sound_disable_cb;
    void* m_on_sound_disable_state;
    player_on_sound_enable_callback m_on_sound_enable_cb;
    void* m_on_sound_enable_state;
    player_on_flush_callback m_on_flush_cb;
    void* m_on_flush_state;
    void*(*m_allocator)(size_t);
    void*(*m_reallocator)(void*,size_t);
    void(*m_deallocator)(void*);
    player(const player& rhs)=delete;
    player& operator=(const player& rhs)=delete;
    void do_move(player& rhs);
    bool realloc_buffer();
public:
    // construct the player with the specified arguments
    player(unsigned int sample_rate = 44100, 
        unsigned short channels = 2, 
        unsigned short bit_depth = 16, 
        size_t frame_count = 256, 
        void*(allocator)(size_t)=::malloc,
        void*(reallocator)(void*,size_t)=::realloc,
        void(deallocator)(void*)=::free);
    player(player&& rhs);
    ~player();
    player& operator=(player&& rhs);
    // indicates if the player has been initialized
    bool initialized() const;
    // initializes the player
    bool initialize();
    // deinitializes the player
    void deinitialize();
    // plays a sine wave at the specified frequency and amplitude
    voice_handle_t sin(unsigned short port, float frequency, float amplitude = .8);
    // plays a square wave at the specified frequency and amplitude
    voice_handle_t sqr(unsigned short port, float frequency, float amplitude = .8);
    // plays a sawtooth wave at the specified frequency and amplitude
    voice_handle_t saw(unsigned short port, float frequency, float amplitude = .8);
    // plays a triangle wave at the specified frequency and amplitude
    voice_handle_t tri(unsigned short port, float frequency, float amplitude = .8);
    // plays RIFF PCM wav data at the specified amplitude, optionally looping
    voice_handle_t wav(unsigned short port, 
                    player_on_read_stream_callback on_read_stream, 
                    void* on_read_stream_state, 
                    float amplitude = .8, 
                    bool loop = false,
                    player_on_seek_stream_callback on_seek_stream = nullptr, 
                    void* on_seek_stream_state=nullptr);
    // plays a custom voice
    voice_handle_t voice(unsigned short port, 
                        voice_function_t fn, 
                        void* state = nullptr);
    // stops a playing voice, or all voices
    bool stop(voice_handle_t handle = nullptr);
    // stops all playing voices on a port
    bool stop_port(unsigned short port);
    // set the sound disable callback
    void on_sound_disable(player_on_sound_disable_callback cb, void* state=nullptr);
    // set the sound enable callback
    void on_sound_enable(player_on_sound_enable_callback cb, void* state=nullptr);
    // set the flush callback (always necessary)
    void on_flush(player_on_flush_callback cb, void* state=nullptr);
    // A frame is every sample for every channel on a given a tick.
    // A stereo frame would have two samples.
    // This is the count of frames in the mixing buffer.
    size_t frame_count() const;
    // assign a new frame count
    bool frame_count(size_t value);
    // get the sample rate
    unsigned int sample_rate() const;
    // set the sample rate
    bool sample_rate(unsigned int value);
    // get the number of channels
    unsigned short channel_count() const;
    // set the number of channels
    bool channel_count(unsigned short value);
    // get the bit depth
    unsigned short bit_depth() const;
    // set the bit depth
    bool bit_depth(unsigned short value);
    // indicates the size of the internal audio buffer
    size_t buffer_size() const;
    // indicates the bandwidth required to play the buffer
    size_t bytes_per_second() {
        return m_sample_rate*m_channel_count*(m_bit_depth/8);
    }
    bool auto_disable() const;
    void auto_disable(bool value);
    bool sound_enabled() const;
    void sound_enabled(bool value);
    // give a timeslice to the player to update itself
    void update();
    // allocates memory for a custom voice state
    template<typename T>
    T* allocate_voice_state() const {
        return (T*)m_allocator(sizeof(T));
    }
};
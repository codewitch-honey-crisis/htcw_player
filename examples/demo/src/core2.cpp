#include <Arduino.h>
#include <driver/i2s.h>
#include <m5core2_power.hpp>
#include <player.hpp>
#include <test.hpp>

m5core2_power power;

player sound(44100,1,16,512);

size_t test_len = sizeof(test_data);
size_t test_pos = 0;
int read_demo(void* state) {
    if(test_pos>=test_len) {
        return -1;
    }
    return test_data[test_pos++];
}
void seek_demo(unsigned long long position, void* state) {
    test_pos = position;
}
void setup() {
    Serial.begin(115200);    
    power.initialize();
    power.speaker_enable(true);
    if(!sound.initialize()) {
        printf("Sound initialization failure.\n");    
        while(1);
    }
    printf("sound throughput: %0.2fkbits/sec\n",sound.bytes_per_second()/1024.0f*8);
    i2s_config_t i2s_config;
    memset(&i2s_config,0,sizeof(i2s_config_t));
    i2s_config.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
    i2s_config.sample_rate = 44100;
    i2s_config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    i2s_config.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
    i2s_config.communication_format = I2S_COMM_FORMAT_STAND_MSB;
    i2s_config.dma_buf_count = 2;
    i2s_config.dma_buf_len = sound.buffer_size();
    i2s_config.use_apll = true;
    i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL2;
    i2s_driver_install((i2s_port_t)I2S_NUM_1, &i2s_config, 0, NULL);
    i2s_pin_config_t pins = {
        .mck_io_num = I2S_PIN_NO_CHANGE,  // Unused
        .bck_io_num = 12,
        .ws_io_num = 0,
        .data_out_num = 2,
        .data_in_num = I2S_PIN_NO_CHANGE};
    i2s_set_pin((i2s_port_t)I2S_NUM_1,&pins);
    
    sound.on_flush([](const void* buffer,size_t buffer_size,void* state){
        size_t written;
        i2s_write(I2S_NUM_1,buffer,buffer_size,&written,portMAX_DELAY);
    });
    sound.on_sound_disable([](void* state) {
        i2s_zero_dma_buffer(I2S_NUM_1);
    });
    sound.wav(0,read_demo,nullptr,.08,true,seek_demo,nullptr);
}
void loop() {
    sound.update();
}
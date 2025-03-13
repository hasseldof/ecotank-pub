#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void initSwitchPort();
unsigned char switchStatus();
unsigned char switchOn(unsigned char switch_nr);

#ifdef __cplusplus
}
#endif
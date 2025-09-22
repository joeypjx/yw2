gcc -O2 -Wall -Wextra \
  -I "/Users/panjinxue/编程/yw2/third_party/openipmi/include" \
  "/Users/panjinxue/编程/yw2/samples/ipmi_lanplus_raw_bridge2.c" \
  "/Users/panjinxue/编程/yw2/third_party/openipmi/lib/.libs/libOpenIPMI.a" \
  "/Users/panjinxue/编程/yw2/third_party/openipmi/unix/.libs/libOpenIPMIposix.a" \
  "/Users/panjinxue/编程/yw2/third_party/openipmi/utils/.libs/libOpenIPMIutils.a" \
  -lpthread -lssl -lcrypto -ldl \
  -o "/Users/panjinxue/编程/yw2/build/ipmi_lanplus_raw_bridge2"
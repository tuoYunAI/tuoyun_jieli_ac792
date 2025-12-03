# 总的 Makefile，用于调用目录下各个子工程对应的 Makefile
# 注意： Linux 下编译方式：
# 1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
# 2. 下载后，解压到 /opt/jieli 目录下，保证
#   /opt/jieli/common/bin/clang 存在（注意目录层次）
# 3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#   可以通过 ulimit -n 8096 来设置一个较大的值
# 支持的目标
# make ac792n_wifi_camera
# make ac792n_wifi_soundbox
# make ac792n_demo_demo_ble
# make ac792n_demo_demo_wifi_ext
# make ac792n_demo_demo_ui
# make ac792n_demo_demo_hello
# make ac792n_demo_demo_wifi

.PHONY: all clean ac792n_wifi_camera ac792n_wifi_soundbox ac792n_demo_demo_ble ac792n_demo_demo_wifi_ext ac792n_demo_demo_ui ac792n_demo_demo_hello ac792n_demo_demo_wifi clean_ac792n_wifi_camera clean_ac792n_wifi_soundbox clean_ac792n_demo_demo_ble clean_ac792n_demo_demo_wifi_ext clean_ac792n_demo_demo_ui clean_ac792n_demo_demo_hello clean_ac792n_demo_demo_wifi

all: ac792n_wifi_camera ac792n_wifi_soundbox ac792n_demo_demo_ble ac792n_demo_demo_wifi_ext ac792n_demo_demo_ui ac792n_demo_demo_hello ac792n_demo_demo_wifi
	@echo +ALL DONE

clean: clean_ac792n_wifi_camera clean_ac792n_wifi_soundbox clean_ac792n_demo_demo_ble clean_ac792n_demo_demo_wifi_ext clean_ac792n_demo_demo_ui clean_ac792n_demo_demo_hello clean_ac792n_demo_demo_wifi
	@echo +CLEAN DONE

ac792n_wifi_camera:
	$(MAKE) -C apps/wifi_camera/board/wl83 -f Makefile

clean_ac792n_wifi_camera:
	$(MAKE) -C apps/wifi_camera/board/wl83 -f Makefile clean

ac792n_wifi_soundbox:
	$(MAKE) -C apps/wifi_soundbox/board/wl83 -f Makefile

clean_ac792n_wifi_soundbox:
	$(MAKE) -C apps/wifi_soundbox/board/wl83 -f Makefile clean

ac792n_demo_demo_ble:
	$(MAKE) -C apps/demo/demo_ble/board/wl83 -f Makefile

clean_ac792n_demo_demo_ble:
	$(MAKE) -C apps/demo/demo_ble/board/wl83 -f Makefile clean

ac792n_demo_demo_wifi_ext:
	$(MAKE) -C apps/demo/demo_wifi_ext/board/wl83 -f Makefile

clean_ac792n_demo_demo_wifi_ext:
	$(MAKE) -C apps/demo/demo_wifi_ext/board/wl83 -f Makefile clean

ac792n_demo_demo_ui:
	$(MAKE) -C apps/demo/demo_ui/board/wl83 -f Makefile

clean_ac792n_demo_demo_ui:
	$(MAKE) -C apps/demo/demo_ui/board/wl83 -f Makefile clean

ac792n_demo_demo_hello:
	$(MAKE) -C apps/demo/demo_hello/board/wl83 -f Makefile

clean_ac792n_demo_demo_hello:
	$(MAKE) -C apps/demo/demo_hello/board/wl83 -f Makefile clean

ac792n_demo_demo_wifi:
	$(MAKE) -C apps/demo/demo_wifi/board/wl83 -f Makefile

clean_ac792n_demo_demo_wifi:
	$(MAKE) -C apps/demo/demo_wifi/board/wl83 -f Makefile clean

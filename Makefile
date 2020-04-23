EXTENDS = /usr/local
ifneq ("as","as${with-extends}")
        EXTENDS =${with-extends}
endif
INSTALL_ROOT = $(EXTENDS)
ifneq ("as","as${prefix}")
        INSTALL_ROOT =${prefix}
endif
##### Change the following for your environment:
COMPILE_OPTS       =	-I./ -I. -O2  -fPIC -DSOCKLEN_T=socklen_t -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
C                  =	c
C_COMPILER         =	cc
C_FLAGS            =	$(COMPILE_OPTS) $(CPPFLAGS) $(CFLAGS)
CPP                =	cpp
CPLUSPLUS_COMPILER =	c++
CPLUSPLUS_FLAGS    =	$(COMPILE_OPTS) -Wall -DBSD=1 $(CPPFLAGS) $(CXXFLAGS)
OBJ                =	o
LINK               =	c++ -o
LINK_OPTS          =	-L. $(LDFLAGS)
CONSOLE_LINK_OPTS  =	$(LINK_OPTS)
LIBRARY_LINK       =	ar cr 
LIBRARY_LINK_OPTS  =	
LIB_SUFFIX         =	a
##### End of variables to change

NAME = libcommon
ALL = $(NAME).$(LIB_SUFFIX)
all:	$(ALL)

COMMON_FLAGS = -DENV_LINUX -D_GNU_SOURCE

.$(C).$(OBJ):
	$(C_COMPILER) -c $(C_FLAGS) $(COMMON_FLAGS) $<
.$(CPP).$(OBJ):
	$(CPLUSPLUS_COMPILER) -c $(CPLUSPLUS_FLAGS) $(COMMON_FLAGS) $<

COMMON_LIB_OBJS = as_mutex.$(OBJ) as_thread.$(OBJ) as_event.$(OBJ) as_json.$(OBJ) \
                  as_queue.$(OBJ) as_queue_safe.$(OBJ) as_time.$(OBJ) as_conn_manage.$(OBJ) \
                  as_daemon.$(OBJ) as_ini_config.$(OBJ) as_lock_guard.$(OBJ) \
                  as_log.$(OBJ) as_onlyone_process.$(OBJ) as_ring_cache.$(OBJ) \
                  as_timer.$(OBJ) as_tinyxml2.$(OBJ) as_http_digest.$(OBJ) as_base64.$(OBJ)

as_mutex.$(C):	as_mutex.h as_config.h as_common.h
as_thread.$(C):	as_thread.h as_config.h as_common.h
as_event.$(C):	as_event.h as_config.h as_common.h
as_json.$(C):	as_json.h as_config.h as_common.h
as_queue.$(C):	as_queue.h as_config.h as_common.h
as_queue_safe.$(C):	as_queue_safe.h as_config.h as_common.h as_mutex.h
as_time.$(C):	as_time.h as_config.h as_common.h
as_http_digest.$(C):	as_http_digest.h as_config.h as_common.h
as_base64.$(C):	as_base64.h as_config.h as_common.h
as_conn_manage.$(CPP):	as_conn_manage.h as_config.h as_common.h
as_daemon.$(CPP):	as_daemon.h as_config.h as_common.h
as_ini_config.$(CPP):	as_ini_config.h as_config.h as_common.h
as_lock_guard.$(CPP):	as_lock_guard.h as_config.h as_common.h
as_log.$(CPP):	as_log.h as_config.h as_common.h
as_ring_cache.$(CPP):	as_ring_cache.h as_config.h as_common.h
as_timer.$(CPP):	as_timer.h as_config.h as_common.h
as_tinyxml2.$(CPP):	as_tinyxml2.h as_config.h as_common.h

$(NAME).$(LIB_SUFFIX): $(COMMON_LIB_OBJS) \
    $(PLATFORM_SPECIFIC_LIB_OBJS)
	$(LIBRARY_LINK)$@ $(LIBRARY_LINK_OPTS) \
		$(COMMON_LIB_OBJS)

clean:
	-rm -rf *.$(OBJ) $(ALL) core *.core *~ include/*~

install: 
	mkdir -p ${EXTENDS}/include/
	mkdir -p ${EXTENDS}/include/common
	mkdir -p ${EXTENDS}/sbin/
	mkdir -p ${EXTENDS}/lib/
	cp *.h   ${EXTENDS}/include/common/
	cp    -d $(NAME).$(LIB_SUFFIX) ${EXTENDS}/lib/

	mkdir -p ${INSTALL_ROOT}/include/
	mkdir -p ${INSTALL_ROOT}/include/common
	mkdir -p ${INSTALL_ROOT}/sbin/
	mkdir -p ${INSTALL_ROOT}/lib/
	cp *.h   ${INSTALL_ROOT}/include/common/
	cp    -d $(NAME).$(LIB_SUFFIX) ${INSTALL_ROOT}/lib/
##### Any additional, platform-specific rules come here:

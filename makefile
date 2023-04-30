server: main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./log/block_queue.h ./noactive/lst_timer.cpp ./noactive/lst_timer.h
	g++ -o server main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./noactive/lst_timer.cpp ./noactive/lst_timer.h -lpthread -Wno-format-truncation


clean:
	rm  -r server
server: main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./log/block_queue.h ./noactive/lst_timer.cpp ./noactive/lst_timer.h CGImysql/sql_connection_pool.cpp CGImysql/sql_connection_pool.h
	g++ -o server main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./noactive/lst_timer.cpp ./noactive/lst_timer.h CGImysql/sql_connection_pool.cpp CGImysql/sql_connection_pool.h -lpthread -Wno-format-truncation -lmysqlclient


clean:
	rm  -r test
	rm  -r server
	rm  -r tmp_log/2023*
	rm */*.h.gch
	

test:main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./log/block_queue.h ./noactive/lst_timer.cpp ./noactive/lst_timer.h CGImysql/sql_connection_pool.cpp CGImysql/sql_connection_pool.h
	g++ -g -o test main.cpp ./threadpool/threadpool.h ./http/http_conn.cpp ./http/http_conn.h ./locker/locker.h ./log/log.cpp ./log/log.h ./noactive/lst_timer.cpp ./noactive/lst_timer.h CGImysql/sql_connection_pool.cpp CGImysql/sql_connection_pool.h -lpthread -Wno-format-truncation -lmysqlclient -fstack-protector-all

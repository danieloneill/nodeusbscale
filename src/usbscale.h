extern "C" {
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <asm/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/hiddev.h>
}
#include <node/uv.h>
#include <nan.h>

#define v8str(a) v8::String::NewFromUtf8(isolate, a)
#define v8num(a) v8::Number::New(isolate, a)

typedef struct _ev_event {
	struct _ev_event	*next;
	uint32_t		hid;
	uint32_t		sync;
	uint32_t		value;
} ev_event;

class UsbScale : public node::ObjectWrap {
	static v8::Persistent< v8::Function > constructor;

        protected:
		uv_thread_t m_thread;
		uv_async_t m_async;

		ev_event	*m_eventQueue;

		int			m_fd;
		Nan::Callback		*m_data;
		bool			m_running;
		bool			m_sync;
		bool			m_transmitted;
		uint32_t		m_lastWeight;

		static void send_event( UsbScale *obj, struct hiddev_event *event );

        public:
                UsbScale();
                ~UsbScale();

		bool initialise( const v8::FunctionCallbackInfo< v8::Value > &args );
	
		// v8 stuff:
                static void Init(v8::Handle<v8::Object> exports);
                static void New(const v8::FunctionCallbackInfo< v8::Value >& args);
		static void Close(const v8::FunctionCallbackInfo< v8::Value >& args);
		static void Pump(const v8::FunctionCallbackInfo< v8::Value >& args);
		static void _Pump(void *arg);
		static void _Callback( uv_async_t *async );
};


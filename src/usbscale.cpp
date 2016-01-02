#ifndef BUILDING_NODE_EXTENSION
# define BUILDING_NODE_EXTENSION
#endif

#include <node/v8.h>
#include <node/node.h>
#include <node/node_object_wrap.h>

#include <string.h>
#include <stdlib.h>
#include <unistd.h>

using namespace v8;

#include "usbscale.h"

#define HID_NOSYNC	9240691
#define HID_SYNC	9240692
#define HID_WEIGHT	9240640

v8::Persistent< v8::Function > UsbScale::constructor;

void UsbScale::Init( v8::Handle<v8::Object> exports )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	
	// Prepare constructor template
	v8::Local<FunctionTemplate> tpl = v8::FunctionTemplate::New(isolate, New);
	tpl->SetClassName(v8str("UsbScale"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	NODE_SET_PROTOTYPE_METHOD(tpl, "pump", Pump);
	NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
	
	constructor.Reset(isolate, tpl->GetFunction());
	exports->Set(v8str("UsbScale"), tpl->GetFunction());
}

void UsbScale::New(const v8::FunctionCallbackInfo<v8::Value>& args) {
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	if( !args.IsConstructCall() )
	{
		// Invoked as plain function `MyObject(...)`, turn into construct call.
		const int argc = 1;
		v8::Local<v8::Value> argv[argc] = { args[0] };
		v8::Local<v8::Function> cons = v8::Local<v8::Function>::New(isolate, constructor);
		args.GetReturnValue().Set(cons->NewInstance(argc, argv));
		return;
	}

	//Invoked as constructor: `new MyObject(...)`
	if (args.Length() < 1) {
		isolate->ThrowException(v8::Exception::TypeError(v8str("Wrong number of arguments.")));
		return;
	}

	UsbScale *obj = new UsbScale();
	if( !obj->initialise( args ) )
	{
		delete obj;
		return;
	}

	obj->Wrap(args.This());
	args.GetReturnValue().Set(args.This());
}

bool UsbScale::initialise( const v8::FunctionCallbackInfo<v8::Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	v8::Local< v8::String > fieldName = args[0]->ToString();
	char fieldChar[ fieldName->Length() + 1 ];
	fieldName->WriteUtf8( fieldChar );
	m_fd = open(fieldChar, O_RDONLY);

	if( m_fd == -1 )
	{
		isolate->ThrowException( v8::Exception::TypeError( v8str("Open failed") ) );
		return false;
	}

	return true;
}

void UsbScale::Close( const v8::FunctionCallbackInfo<Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	UsbScale* parent = Unwrap<UsbScale>(args.Holder());
	parent->m_running = false;
	uv_thread_join( &parent->m_thread );
	uv_close( (uv_handle_t*)&parent->m_async, NULL );
}

void UsbScale::Pump( const v8::FunctionCallbackInfo<Value>& args )
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);

	UsbScale* parent = Unwrap<UsbScale>(args.Holder());
	if( args.Length() > 0 )
	{
		v8::Local<v8::Function> callbackHandle = args[0].As<Function>();
		parent->m_data = new Nan::Callback(callbackHandle);
	}

	uv_async_init( uv_default_loop(), &parent->m_async, _Callback );
	parent->m_async.data = (void*)parent;

	parent->m_running = true;
	uv_thread_create( &parent->m_thread, _Pump, parent );
}

void UsbScale::send_event( UsbScale *e, struct hiddev_event *ev )
{
	// Enqueue the event:
	ev_event *n = new ev_event;
	n->next = NULL;
	n->hid = ev->hid;
	n->sync = e->m_sync;
	n->value = e->m_lastWeight;
	
	if( !e->m_eventQueue )
		e->m_eventQueue = n;
	else
	{
		ev_event *c = e->m_eventQueue;
		while( c && c->next ) c = c->next;
		c->next = n;
	}
	
	// Notify the event loop:
	uv_async_send( &e->m_async );
}

void UsbScale::_Pump(void *arg)
{
	UsbScale *e = static_cast<UsbScale *>(arg);
	struct hiddev_event ev;

	ssize_t rc = 0;
	do {
		rc = read( e->m_fd, &ev, sizeof(ev) );
		if( rc == -1 )
		{
			perror("read");
		}
		else if( rc < sizeof(ev) )
		{
			// Short read.
			printf("Short read!\n");
		}
		else if( ev.hid == HID_NOSYNC )
		{
			e->m_sync = false;
			send_event( e, &ev );
		}
		else if( ev.hid == HID_SYNC )
		{
			e->m_sync = true;
			send_event( e, &ev );
		}
		else if( ev.hid == HID_WEIGHT && e->m_lastWeight != ev.value )
		{
			e->m_lastWeight = ev.value;
			send_event( e, &ev );
		}
	} while( rc >= 0 && e->m_running );

	perror("USBScale: Read error:");
	return;
}

void UsbScale::_Callback(uv_async_t *uv)
{
	v8::Isolate* isolate = v8::Isolate::GetCurrent();
	v8::HandleScope scope(isolate);
	UsbScale *e = static_cast<UsbScale *>(uv->data);
	
	ev_event *queue = e->m_eventQueue;
	e->m_eventQueue = NULL;

	if( !e->m_data )
		return;

	// Not the fastest way:
	int length = 0;
	for( ev_event *c=queue; c; c=c->next ) length++;

	// Build result array:
	v8::Local<v8::Array> results = v8::Array::New(isolate, length);

	int idx = 0;
	for( ev_event *c=queue; c; c=c->next )
	{
		v8::Local<v8::Object> event = v8::Object::New(isolate);
/*
		if( c->hid == HID_NOSYNC )
			event->Set( v8str("balanced"), v8::Boolean::New(isolate, false) );
		else if( c->hid == HID_SYNC )
			event->Set( v8str("balanced"), v8::Boolean::New(isolate, true) );
		else if( c->hid == HID_WEIGHT )
			event->Set( v8str("weight"), v8num( c->value ) );
*/
		event->Set( v8str("balanced"), v8::Boolean::New(isolate, c->sync) );
		event->Set( v8str("weight"), v8num( c->value ) );

		results->Set( idx++, event );
	}

	const int argc = 1;
	v8::Local<v8::Value> argv[argc] = { results };

	Nan::MakeCallback(Nan::GetCurrentContext()->Global(), e->m_data->GetFunction(), argc, argv);

	ev_event *n = queue;
	for( ev_event *c=n; c; c=n )
	{
		n = c->next;
		delete c;
	}
}

UsbScale::UsbScale() :
	m_eventQueue(NULL),
	m_fd(0),
	m_data(NULL),
	m_running(false),
	m_sync(false),
	m_lastWeight(0)
{
}

UsbScale::~UsbScale()
{
	if( m_running )
	{
		m_running = false;
		uv_thread_join( &m_thread );
		uv_close( (uv_handle_t*)&m_async, NULL );
		close( m_fd );
		delete m_data;
	}
}

// "Actual" node init:
static void init(v8::Handle<v8::Object> exports) {
	UsbScale::Init(exports);
}

// @see http://github.com/ry/node/blob/v0.2.0/src/node.h#L101
NODE_MODULE(nodeusbscale, init);

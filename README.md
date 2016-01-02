# nodeusbscale
Node.js+Linux (HID) bindings for several USB scales (Pitney Bowes, Dymo, etc.)

Tested with:
* 0922:8004 Dymo-CoStar Corp.
* 0d8f:0200 Pitney Bowes

Data packets are in the following format:
```javascript
{
	'weight': xxxxxx,
	'balanced': <true/false>
}
```

### Example

```javascript
#!/usr/bin/env node

var UsbScale = require('nodeusbscale');

var dev = UsbScale.create();
dev.open( '/dev/usb/hiddev0', function(obj, err) {
	if( err ) {
		console.log("UsbScale: Failed to open device: "+err);
		return false;
	}
	dev['data'] = function(data) {
		console.log("UsbScale event: "+JSON.stringify(data, null, 2));
	};
});

// Hack to prevent Node.js from exiting, sorry.
var http = require('http');
var httpServer = http.createServer(function(){});
httpServer.listen(8084);
console.log("Listening for keyboard input...");
```

# Methods

## Factory

### usbscale::create()
`Returns an initialised usbscale object.`

* Returns: An initialised usbscale object.

---

## usbscale object

### usbscaleObject::open( path, callback( usbscaleObject, error ) )
`Open an usbscale device, associate it to this object, and fire the provided callback upon completion.`

The parameters passed to the provided callback function are **usbscaleObject** which is a reference to the usbscale object, and **error** which will be an error string in the case of an error, or undefined on success.

* Returns: A reference to itself.

### usbscaleObject::data( usbscaleEvent )
`A virtual method of the usbscaleObject. Reimplement it to handle usbscaleEvents. It can be ignored, but if you ignore it, you might as well not use this module at all.`


var UsbScale = require('./build/Release/usbscale.node');
var factory = {
	'create': function()
	{
		var self = this;
		var obj = {
			'stream': false,
			'ev': null,
			'options': {
				'flags': 'r',
				'encoding': null,
				'fd': null,
				'autoClose': true
			},

			'data': function( obj )
			{
				// No-op by default. Set this yourself on the returned object.
			},

			'open': function(path, callback)
			{
				try
				{
					this.ev = new UsbScale.UsbScale( path );
					if( callback )
						callback(this);

					this.ev.pump(this.data);
				} catch(e) {
					if( callback )
						callback(this, e);
				}
				return obj;
			},

			'close': function()
			{
				this.ev.close();

				delete this.ev;
			}
		};
		
		return obj;
	}
};
module.exports = factory;

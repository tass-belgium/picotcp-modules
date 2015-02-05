picotcp-modules
===============

Application level modules to run on top of the popular Embedded TCP/IP stack


### HTTPlib HOWTO:
 Accepted parameters for HTTPLib are:

* PICOTCP=/path/to/picotcp/sources
* ARCH=any supported picotcp arch
* CROSS_COMPILE=your-compiler-prefix-here-
* FEATURES= other command line parameters that will be passed to PicoTCP compilation


 The result of the make command  will be libhttp/libhttp.a, which *ALREADY* contains picotcp symbols, and it is ready to be linked to your pico_http application


in doubt? contact us at info _at_ picotcp _dot_ com


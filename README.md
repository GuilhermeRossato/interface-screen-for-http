# [INCOMPLETE] Screen Interface via HTTP Program for Windows

A project for building a windows 10 utility tool to serve screen regions (pixels) via the HTTP protocol, in both clear text and binary formats.

It is designed to answer a request as fast as possible, therefore it will use DirectX9 to request a screen region for the OS so that it can be read easily by other applications.

## How to use

Download a release from the [releases page](https://github.com/GuilhermeRossato/interface-screen-for-http/releases) and run it. It should open a server at port `8081` by default, use a browser to access http://localhost:8081/ and you should get a PNG image served by this tool! Check the standard and error output to check for errors if anything went wrong.

If you want you can provide a port via argument to change it.

## Why is it useful

Suppose you want to capture a region of your screen and read its color in your programming language, what if it was as easy as sending the following HTTP request:

```http
GET /region/?monitor=0&x=0&y=100&width=1000&format=png
```

And the program answered it with:

```
HTTP/1.1 200 OK
Date: Sat, 09 Oct 2020 14:28:25 GMT
Content-Type: application/png
Content-Length: 10335
// actual png file binary data
```

Or maybe you request something like:

```http
GET /pixel/?x=10&y=10&format=text&content=r,g,b
```

In english: Request the pixel at 10, 10 on the default monitor, in the text format with the three primary colors r, g, b separated by commas, which would yield this:

```
HTTP/1.1 200 OK
Date: Sat, 09 Oct 2020 14:28:25 GMT
Content-Type: plain/text
Content-Length: 10

120,26,255
```

This program that creates a network interface, a html server that can reply with binary or textual data of the screen, for whatever reason, maybe I want to serve my screen in the web directly, or a program wants to detect something on my screen, anything.

The idea is that this program knows how to fetch screen pixels, it is specialized in the specific problem of getting pixel colors of the screen into some other program that communicates via the HTTP interface.

## Features

This tool will be implemented in terms of features, one by one:

1. It must reply a full image file by default containing the pixels of the screen at the current time.
2. It must allow for parameters to change the size and position of the region being requested
3. It must allow individual pixel requests in both binary and text format


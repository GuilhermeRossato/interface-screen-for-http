# Screen Interface via HTTP Program for Windows

A project for building a working windows program to serve screen regions (pixels) via the HTTP protocol, in both clear text and binary formats by exposing a server in a port.

It is designed to answer a request as fast as possible, therefore it will use DirectX9 to request a screen region for the OS so that it can be read easily by other applications.

## Features

1. Pass a number by parameter to change the default port number as you run it.
2. Root (http://localhost:8081/) replies with a full screen picture of the default screen
3. Single pixel requests at http://localhost:8081/pixel/
    Parameters: `x`, `y` and `format`

4. Region request at http://localhost:8081/
    parameters: `x` (defaults 0), `y` (defaults 0), `width` (defaults full), `height` (defaults full) and `format`

5. `format` parameters allows for data to come in three main formats:

    5.1. "`png`" (or `image`) to output a uncompressed PNG file of the screen content

    5.2. "`binary`" (or `bin`) to output a binary data where each 3 bytes is the byte value of red, green and blue, respectivel

    5.3. "`text`" (or `txt`) to output a text separating channels by comma, and pixels by line.

    5.4. "`json`" (or `js`) to output a stringfied JSON object with the keys `r`, `g`, `b`, `x` and `y`. Note that in this format only a single pixel of data is returned (region json is not implemented as of now)

Obs: I have made so that `x` and `y` can be called `left` and `top` in the query string, as that was a common typo.

## Request URL Examples

URL to request a full screen png image of the screen as it is now:

http://localhost:8081/

URL to request a single pixel in JSON format (only single-pixel is supported for that format):

http://localhost:8081/pixel/?x=10&y=10&format=json

URL to request a 100 by 100 region offseted by 10 pixels at the left and 20 pixels from the top:

http://localhost:8081/?left=10&top=20&width=100&left=100

URL to request a 5 by 5 region offseted by 10 pixels from the top left origin and in the text format (`R,G,B\nR,G,B\n...`), left to right:

http://localhost:8081/?x=10&y=10&width=5&height=5&format=text

URL to request 10 pixels in a row at the top left origin (0, 0) in the binary format. `Content-length` will be 30 bytes in a left-to-right contiguouus array of `RGB` groups, not separated by anything (`RGBRGBRGB...`):

http://localhost:8081/?width=10&height=1&format=bin

## Utility endpoint to get screen size

Requests to this endpoint will return the width and the height of the screen separated by a comma respectively:

http://localhost:8081/size/

Should give you something like:

```
2560,1080
```

## How to use

Download a release from the [releases page](https://github.com/GuilhermeRossato/interface-screen-for-http/releases) and run it. It should open a server at port `8081` by default, use a browser to access http://localhost:8081/ and you should get a PNG image served by this tool! Check the standard and error output to check for errors if anything went wrong.

If you want you can provide a port via argument to change it.

## Why is it useful

Suppose you want to capture a region of your screen and read its color in your programming language, what if it was as easy as sending the following HTTP request:

```http
GET /?monitor=0&x=0&y=100&width=1000&format=png
```

And the program answered it with:

```
HTTP/1.1 200 OK
Content-Type: image/png
Content-Length: 10335
// actual png file binary data
```

Or maybe you request something like a single pixel in a specific format:

```http
GET /pixel/?x=10&y=10&format=text
```

Roughly translate to: "Request the pixel at 10, 10 on the default monitor, in the text format", which would yield this:

```
HTTP/1.1 200 OK
Content-Type: plain/text
Content-Length: 10

120,26,255
```

The three primary colors r, g, b are separated by commas, in plain text for any other program to use it.

This program creates a network interface for a http server that can reply with binary or textual data of the screen, for whatever reason, from serving my screen in the web directly, or a program wants to detect something on my screen, anything.

The idea is that this program knows how to fetch screen pixels, it is specialized in the specific problem of getting pixel colors of the screen into some other program that communicates via the HTTP interface.

## Features

This tool will be implemented in terms of features, one by one:

1. Must reply a full image file by default containing the pixels of the screen at the current time. `[Done]`
2. Must allow for parameters to change the size and position of the region being requested `[Done]`
3. Must allow individual pixel requests in both binary and text format `[Done]`
4. Must allow a JSON format for one pixel `[Done]`
5. Utility function to reply screen size `[Done]`

# Disclaimer

The project is finished, I do not make any warranty about the working state of the software included in this repository. I also shall not be responsible for any harm the use of the software might cause.

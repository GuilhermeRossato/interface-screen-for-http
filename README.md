# DirectX Screen Interface Program

In windows, sometimes, I want to have a program that captures my screen. Each language has its own way to do that, libraries, modules, whatever.

If I create a program that creates a network interface i.e. a html server that can feed binary or simplified data about the screen, I could access it using anything else, including in-browser.

The idea is that this program knows how to fetch screen pixels, the area and what not and it handles that part so that my other tools don't have to.

## Features

This tool will be implemented in terms of features, one by one:

1. It must support debug mode by checking a `debug.txt` file routinely as it runs - writing to `log.txt` everything that happens to it, as it will be a service.
2. It must allow for configuration of where `debug.txt` file is and `log.txt` file is by command line parameters.
3. It must reply in a configurable port for request of available screens.
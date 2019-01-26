---
title: Py.js Documentation
---

# py.js

Node.js/Python Bridge; Node.js-hosted Python.

## Introduction

<p class="warning no-bg">
	*This project is currently a WIP (work-in-progress) and is currently in **alpha**. Significant changes are to come.*
</p>

*Tested on Linux, Mac OS, and Windows 10, with Python 3.7 and Node.js 11.7.*

Py.js is written using modern C++, and hosts a Python interpreter directly within Node's Javascript engine, allowing Node.js interoperability with Python and Python libraries. This add-on also utilizes Node's N-API, which provides a general guarantee of stability between Node versions, such that py.js will not have to be rewritten anytime a new version of Node.js is released. **Py.js currently only supports Python 3.**

*Released under the AGPLv3.*

``` js
const p = require('py.js')
p.init()
p.base().print('Hello World from Python!')
```

Please see below for caveats and general design philosophy.

### Installation

Installation is quick and simple.

#### Linux & Mac

```
npm install py.js
```

The module should automatically compile on Linux and Mac without any additional configuration. If there is an issue, please confirm that ```python3-config``` exists, and that you have Python 3 installed.

#### Windows

Ensure that npm is using the correct version of Python. At current, node-gyp only supports Python 2. Windows also requires the *windows-build-tools*. See [node-gyp](https://github.com/nodejs/node-gyp) for more details.

##### Example Configuration Path
```
npm config set python C:\Python27\python.exe
```

Next, include the path to your Python 3 directory when issuing npm the command to install. Please ensure that the version of Python you are using matches your system's architecture. In most cases, you’ll want the 64-bit version of Python.

##### Running npm on Windows
```
npm install --python_include="C:\\Python37"
```

<p class="warning no-bg">
  Please note that the directory path uses escaped backslashes (\\). This format is required by node-gyp.
</p>


<!--## Py.js Design-->


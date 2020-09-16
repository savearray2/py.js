---
title: Py.js Documentation
---

# py.js

Node.js/Python Bridge; Node.js-hosted Python.

## Introduction

<p class="warning no-bg">
	*This project is currently a WIP (work-in-progress) and is currently in **alpha**. Significant changes are to come.*
</p>

Py.js is written using modern C++, and hosts a Python interpreter directly within Node's Javascript engine, allowing Node.js interoperability with Python and Python libraries. This add-on also utilizes Node's N-API, which provides a general guarantee of stability between Node versions, such that py.js will not have to be rewritten every time a new version of Node.js is released. **Py.js currently only supports Python 3.**

*Tested on Linux, Mac OS, and Windows 10. See our build tests here: [Linux & Mac](https://travis-ci.org/savearray2/py.js), [Windows](https://ci.appveyor.com/project/savearray2/py-js).*

*<small>Released under the AGPLv3.</small>*

``` js
const p = require('@savearray2/py.js')
p.init()
p.base().print('Hello World from Python!')
```

Please see below for caveats and general design philosophy.

### Installation

Installation is quick and simple.

#### Linux & Mac

```
npm install @savearray2/py.js
```

The module should automatically compile on Linux and Mac without any additional configuration. If there is an issue, please confirm that ```python3-config``` exists, and that you have Python 3 installed. Version ```g++6``` or higher is recommended for Linux.

#### Windows

Ensure that ```npm``` is using the correct version of Python. At current, ```node-gyp``` only supports Python 2. Windows also requires the *windows-build-tools*. See [node-gyp](https://github.com/nodejs/node-gyp) for more details.

##### Example Configuration Path
```bash
npm config set python C:\Python27\python.exe
```

Next, include the path to your Python 3 directory when issuing ```npm``` the command to install. Please ensure that the version of Python you are using matches your system's architecture. In most cases, you’ll want the 64-bit version of Python.

##### Running ```npm``` on Windows
```
npm install @savearray2/py.js --python_include="C:\\Python37"
```

<p class="warning no-bg">
  Please note that depending on your environment, the directory path may require the use of escaped backslashes (\\) for ```node-gyp``` to parse your Python 3 path properly.
</p>

You should now be able to use py.js. Make sure that you have the Python directory you used to link & compile py.js included in your Windows ```PATH``` string. For example:

```
SET PATH=C:\\Python37-x64;%PATH%
```


### Using ```pyenv``` during installation 

```Pyenv``` can be utilized to specify a Python version to use with py.js.

#### Linux & Mac

Navigate to the directory you wish to install py.js, and install the desired Python version with the following command:

```bash
PYTHON_CONFIGURE_OPTS="--enable-shared" pyenv install 3.6.2
```

Once ```pyenv``` compiles Python, use the following command to set the local version of Python for your folder:

```bash
pyenv local 2.7.15 3.6.2
```

<p class="warning no-bg">
  ```npm``` and ```node-gyp``` require Python 2 to function. You may need to specify ```npm config set python python``` to ensure ```npm``` is using the correct Python executable. Using a Python 2 version first in the ```pyenv local``` command creates a symlink from ```python``` to Python 2. The Python 3 version selected will still be accessible through the ```python3``` command.
</p>

Finally, run the install like normal.

```bash
npm install @savearray2/py.js
```

It is possible that Linux, or other POSIX compliant systems, may have trouble finding the shared library produced by ```pyenv```. If this occurs, adjust your environment variables to include the proper ```LD_LIBRARY_PATH```.

```bash
export LD_LIBRARY_PATH="$(python3-config --prefix)/lib:$LD_LIBRARY_PATH"
```

Some distributions may also use different utilities and methodologies to find shared libraries. For example, Ubuntu uses ```ldconfig```. Please refer to your distribution's manual for more information, and for the best way to reference shared libraries on your system. There also may be certain security implications depending on the method you pick.

If you wish, you may also link the shared Python library during the  ```node-gyp``` build process with a ```RPATH``` or ```RUNPATH``` reference. The *LDFLAGS* and *CFLAGS* options are configurable through the ```python_config_cflags``` and ```python_config_ldflags``` command-line flags. Here is an example of how they might be used:

```bash
npm install --verbose --python_config_ldflags="$(python3-config --ldflags) -Wl,--enable-new-dtags -Wl,-R,$(python3-config --prefix)/lib" --python_config_cflags="$(python3-config --cflags)"
```

These flags will not work on OS X. Please see the project's ```binding.gyp``` file for more information.

You can check the version of Python that py.js is using with the following short script:

```js
const p = require('@savearray2/py.js')
console.log(`${p.init().instance().python_version}`)
p.finalize()
```

```
3.7.2 (default, Jan 10 2019, 23:51:51) 
[GCC 8.2.1 20181127]
```

Now when you run your Node.js script from the folder you have designated, py.js should also use the Python 3 version you have specified.

#### Using ```virtualenv``` or ```venv```

The virtualenv plugin for pyenv does not seem to create a symlink to ```python-config``` or ```python3-config```. As such, ```node-gyp``` will fail during the build process, as it will not be able to find the necessary information for compilation and linking. We suggest that you build under a specific ```pyenv``` version first, and then apply a ```virtualenv``` container after build completion. For example:

```bash
cd my_work_directory
pyenv local 2.7.15 3.7.2
npm install @savearray2/py.js
pyenv virtualenv 3.7.2 pyjs
pyenv local pyjs
```

### Getting Started

To get started using the library, import it like usual:

```js
const p = require('@savearray2/py.js')
```

Before the Python interpreter can be accessed, py.js must be initialized. Py.js will throw an exception if the ```init``` function is called multiple times. Py.js will also throw an exception if you attempt to access the Python interpreter before initialization. The library can be initialized very easily.

```js
const p = require('@savearray2/py.js')
p.init({
	pythonPath: 
		`${process.cwd()}:/usr/local/lib/python3.7/site-packages`
})
```

The ```init``` function takes an options object parameter with some useful properties. When a Python interpreter is embedded, it must be instructed where to load its base library files. This is typically done with environment variables. The ```PYTHONPATH``` environment variable can be altered through the init function, as shown above. One of the easiest ways to figure out the default library paths for a given Python runtime is by invoking Python itself.

```js
const exec = require('child_process').execSync
const site = exec('python3 -c "from distutils.sysconfig import get_python_lib; print(get_python_lib())"',
	{encoding: 'utf8'}).trim() //Make sure you include trim() to remove extraneous whitespace
```

It is also wise to include the current directory when setting ```PYTHONPATH```. Without doing so, it may not be possible for the embedded Python runtime to find Python scripts you have included in the same directory as your Node.js scripts. *Py.js will not attempt to set ```PYTHONPATH``` on its own.*


### Design Limitations

.

---

## API List (Subject to change)

### pyjs

##### pyjs.init([options])

##### pyjs.finalize()

##### pyjs.instance()

##### pyjs.base()

Built-in objects. Base functionality that Python typically loads automatically. Equivalent to ```__builtins__```. 

##### pyjs.$GetCurrentThreadID()

##### pyjs.import()

##### pyjs.eval()

##### pyjs.evalAsFile()

#### pyjs.$coerceAs

##### pyjs.$coerceAs.Integer

Shortcuts: ```pyjs.$coerceAs.int```

<!--## Py.js Design-->


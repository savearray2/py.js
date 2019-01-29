# py.js

[![Build status](https://ci.appveyor.com/api/projects/status/e21hoqmy6lgre65w/branch/testing?svg=true)](https://ci.appveyor.com/project/savearray2/py-js/branch/testing) [![Build Status](https://travis-ci.org/savearray2/py.js.svg?branch=master)](https://travis-ci.org/savearray2/py.js)

Node.js/Python Bridge; Node.js-hosted Python.

Documentation can be found [here](https://savearray2.github.io/py.js/).
Click here for [here](examples/README.md) a few basic examples.

*This project is currently a WIP (work-in-progress) and is currently in **alpha**. Significant changes are to come.*

<p align="center"><img src="https://savearray2.github.io/py.js/static/1.png" height="400" /></p>

```js
let chalk = require('chalk')
let p = require('py.js')
p.init({ pythonPath: 
	`${process.cwd()}/local:/usr/local/lib/python3.7/site-packages`
})

let plotly = p.import('plotly')
let np = p.import('numpy')
let go = plotly.graph_objs
let pio = plotly.io

let [x,y,colors,sz] = 
	[0,0,0,0].map(() => 
		np.random.rand(p.$coerceAs.int(100)))
sz = sz.$mul(p.$coerceAs.int(30))
let fig = go.Figure()
fig.add_scatter.$apply({
	x: x, y: y, mode: 'markers',
	marker: {
		size: sz, color: colors,
		opacity: 0.6, colorscale: 'Viridis'
	}
})

pio.write_image(fig, 'image.png')
console.log(chalk`{cyan Success:} Chart saved!`)
```

#### Recommended Configuration

* Node.js >= v10.0.0
* Python >= v3.5
* C++14 compatible compiler, or better

Note: *x86 and ARM architectures have yet to be tested.*

#### Current Testing Matrix

##### Linux
* Ubuntu 14.04 LTS, g++6, Node.js 10 LTS, Python 3.5 (x64)
* Ubuntu 16.04 LTS, g++8, Node.js 10 LTS, Python 3.7 (x64)
* Ubuntu 16.04 LTS, g++8, Node.js 11, Python 3.7 (x64)

##### OS X
* OS X 10.13, Node.js 11, Python 3.7 (x64)
* OS X 10.14, Node.js 11, Python 3.7 (x64)

##### Windows
* Windows 10, Node.js 10 LTS, Python 3.7, MSVS 2015 (x64)
* Windows 10, Node.js 11, Python 3.7, MSVS 2015 (x64)

*See our build tests here: [Linux & Mac](https://travis-ci.org/savearray2/py.js), [Windows](https://ci.appveyor.com/project/savearray2/py-js).*


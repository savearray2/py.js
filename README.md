_I'm a bit busy at the moment, but have launched the library in a basic form. The library functions properly, but the async code that handles Node functions being asynchronously called from Python needs to be completed. The code should be cleaned up a bit, as well. If there's community demand, I'll try to devote more time to the library._

# py.js

[![Build status](https://ci.appveyor.com/api/projects/status/e21hoqmy6lgre65w/branch/testing?svg=true)](https://ci.appveyor.com/project/savearray2/py-js/branch/testing) [![Build Status](https://travis-ci.org/savearray2/py.js.svg?branch=master)](https://travis-ci.org/savearray2/py.js)

Node.js/Python Bridge; Node.js-hosted Python.

Documentation can be found [here](https://savearray2.github.io/py.js/).
Click here for [here](examples) a few basic examples.

*This project is currently a WIP (work-in-progress) and is currently in **alpha**. Significant changes are to come.*

<p align="center"><img src="https://savearray2.github.io/py.js/static/1.png" height="400" /></p>

```js
let chalk = require('chalk')
let p = require('@savearray2/py.js')
p.init({ pythonPath: 
	`${process.cwd()}/local:/usr/local/lib/python3.8/site-packages`
})

let plotly = p.import('plotly')
let np = p.import('numpy')
let go = plotly.graph_objs
let pio = plotly.io

let [x,y,colors,sz] = 
	[0,0,0,0].map(() => 
		np.random.rand(100n))
sz = sz.__mul__(30n)
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

* Node.js >= v10.4.0
* Python >= v3.5
* C++14 compatible compiler, or better

Note: *x86 and ARM architectures have yet to be tested.*

#### Current Testing Matrix

##### Linux
* Ubuntu 14.04 LTS, g++6, Node.js 14 LTS, Python 3.5 (x64)
* Ubuntu 16.04 LTS, g++8, Node.js 14 LTS, Python 3.9 (x64)
* Ubuntu 16.04 LTS, g++8, Node.js 15, Python 3.9 (x64)

##### OS X
* OS X 10.15, Node.js 14, Python 3.9 (x64)
* OS X 11.1, Node.js 15, Python 3.9 (x64)

##### Windows
* Windows 10, Node.js 14 LTS, Python 3.9, MSVS 2019 (x64)
* Windows 10, Node.js 15, Python 3.9, MSVS 2019 (x64)

*See our build tests here: [Linux & Mac](https://travis-ci.org/savearray2/py.js), [Windows](https://ci.appveyor.com/project/savearray2/py-js).*

#### License

This project is licensed under a modified version of the AGPLv3, which is intended to allow linking (colloquially known as the ALGPL, or Lesser Affero GPLv3). In short, if you make improvements to this library, please share them!

```
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.

Additional permission under the GNU Affero GPL version 3 section 7:

If you modify this Program, or any covered work, by linking or
combining it with other code, such other code is not for that reason
alone subject to any of the requirements of the GNU Affero GPL
version 3.
```

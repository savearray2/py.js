# py.js
Node.js/Python Bridge; Node.js-hosted Python.

*This project is currently a WIP (work-in-progress) and is currently in **alpha**. Significant changes are to come.*

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
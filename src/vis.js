/*
var tfix = RegExp(/^time\s+fixed\s+T[0-9]+=[0-9]+(\.[0-9]+)?$/);
var tfree = RegExp(/^time\s+free\s+T[a-z]+=[0-9]+(\.[0-9]+)?$/);
var nfix = RegExp(/^time\s+fixed\s+2N[a-z]+=[0-9]+(\.[0-9]+)?$/);
var nfree = RegExp(/^time\s+free\s+2N[a-z]+=[0-9]+(\.[0-9]+)?$/);
var nconstr = RegExp();
var comment = RegExp("#.*");
*/

inp =
`0x7fff589873c8 twoN=120.000000 ntrval=(5.500000,Inf)
   0x7fff58987270 twoN=96.400000 ntrval=(3.000000,5.500000)
      0x7fff58986d10 twoN=100.000000 ntrval=(0.000000,3.000000)
      0x7fff58987118 twoN=32.100000 ntrval=(1.000000,3.000000)
         0x7fff58986e68 twoN=123.000000 ntrval=(0.000000,1.000000)
   0x7fff58986fc0 twoN=213.400000 ntrval=(1.000000,5.500000)
      0x7fff58986e68 twoN=123.000000 ntrval=(0.000000,1.000000)`

class Node {
	constructor(two_n, ntrval_a, ntrval_b, parent) {
		this.two_n = two_n;
    this.ntrval_a = ntrval_a;
    this.ntrval_b = ntrval_b;
    this.parents = [];
    if (parent) {
    	this.add_parent(parent);
    }
    this.children = [];
	}

  add_parent(parent) {
  	if (this.parents.length > 1) {
    	throw new Exception("attempted to add third parent");
    }
    this.parents.push(parent);

    if (parent.children.length > 1) {
    	throw new Exception("attempted to add third child");
    }
    parent.children.push(this);
  }
}

lines = inp.split('\n');
nodes = {};
cur_parents = [];

for (i = 0; i < lines.length; i++) {
	nspace = lines[i].search(/\S/);
  console.assert(nspace >= 0);
  console.assert(nspace % 3 == 0);
  console.log(lines[i]);
  depth = nspace / 3;
  console.assert(cur_parents.length >= depth)
  console.log("depth:", depth);

  full_regex = /^\s*(0x[0-9a-f]{12})\s+twoN=[0-9]+.[0-9]+ ntrval=\([0-9]+.[0-9]+,([0-9]+.[0-9]+|Inf)\)/;
  console.assert(lines[i].match(full_regex));

  var [addr, twon_str, ntrval_str] = lines[i].trim().split(/\s+/);
  var two_n = parseFloat(twon_str.substring(5));
  var [ntrval_a_str, ntrval_b_str] = ntrval_str.substring(8, ntrval_str.length-1).split(',');
  var ntrval_a = parseFloat(ntrval_a_str);

  if (ntrval_b_str.localeCompare("Inf") == 0) {
  	ntrval_b = Infinity;
  } else {
  	var ntrval_b = parseFloat(ntrval_b_str);
  }

  /*
  console.log(ntrval_a_str);
  console.log(ntrval_b_str);
  console.log(addr);
  console.log("twoN:", two_n);
  console.log("ntrval:", ntrval_a, ntrval_b);
  */

  var parent = null;
  if (depth > 0) {
  	parent = cur_parents[depth-1];
  }

  if (!nodes[addr]) {
  	nodes[addr] = new Node(two_n, ntrval_a, ntrval_b, parent);
  } else {
  	nodes[addr].add_parent(parent);
  }
  console.log(nodes[addr]);
  console.log();

  if (cur_parents.length = depth) {
  	cur_parents.push(nodes[addr]);
  } else {
  	cur_parents[depth] = nodes[addr];
  }
}

for (var key in nodes) {
	console.log(key);
  console.log(nodes[key]);
}

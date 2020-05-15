/*
 Agent class.  An agent is a separate individual that communicates via messages to other agents.

 It contains a 3D representation implemented via three.js classes
 It contains a 2D parameter setup representation implemented via React components
 */
export class Agent {
  marked = false;

  constructor(context,radio,id,world,size,x,y,z,rotx=0,roty=0,rotz=0) {
    this.THREE = context;
    this.radio = radio;
    this.id = id;
    this.size = size;
    this.world = world;
    this.x = x;
    this.y = y;
    this.z = z;
    this.rotx = rotx;
    this.roty = roty;
    this.rotz = rotz;
    this.state = {};

    this.createMesh();
    this.neighbors = [];

    // Validate to catch runtime bugs
    if (typeof id === "undefined") {
      throw new Error("Agent.id must be set");
    }

    if (typeof radio === 'undefined') {
      throw new Error("Agent.radio must be set");
    }
  }

  setMarked(val) {
    this.marked = val;

    for (let i = this.group.children.length - 1; i >= 0; i--) {
      this.group.remove(this.group.children[i]);
    }

    this.group.add(this.mesh);

    if (!this.marked) return;

    let geometry = new this.THREE.SphereGeometry(this.size*1.3, 24, 24,0,Math.PI*2,0,Math.PI/2);
    let material = new this.THREE.MeshBasicMaterial({
      color: 0xFF0000,
    });
    let marker = new this.THREE.Mesh(geometry, material);
    marker.position.copy(new this.THREE.Vector3(this.x,this.y,this.z-this.size));
    marker.rotation.x = this.rotx;
    marker.rotation.y = this.roty;
    marker.rotation.z = this.rotz;

    this.group.add(marker);
  }

  getMarked() {
    return this.marked;
  }

  // State should always be a single map for easier serialization
  getState() {
    return this.state;
  }

  getPosition() {
    return new this.THREE.Vector3(this.x,this.y,this.z);
  }

  createMesh() {
    this.geometry = new this.THREE.SphereGeometry(this.size, 24, 24,0,Math.PI*2,0,Math.PI/2);
    this.material = new this.THREE.MeshBasicMaterial({
      color: 0xFFFFFF,
    });
    this.mesh = new this.THREE.Mesh(this.geometry, this.material);
    this.mesh.position.copy(new this.THREE.Vector3(this.x,this.y,this.z));
    this.mesh.rotation.x = this.rotx;
    this.mesh.rotation.y = this.roty;
    this.mesh.rotation.z = this.rotz;

    this.group = new this.THREE.Group();
    this.group.add(this.mesh);
  }

  // Called only once.  You may modify it but cannot reallocate it.
  getMesh() {
    return this.group;
  }

  // Simulation methods
  loop(time) {

  }

  setNeighbors(val) {
    this.neighbors = [...val];
  }

  getNeighbors() {
    return this.neighbors;
  }

  addNeighbor(id) {
    this.neighbors.push(id);
  }

  removeNeighbor(id) {
    let index = this.neighbors.indexOf(id);
    if (index !== -1) this.neighbors.splice(index,1);
  }

  receiveMessage(msg) {
  }

  // simulator.sendMessage(id,msg) - Sends a message to another Agent.  Has a variable delay
}

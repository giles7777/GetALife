import React, { Component, memo } from "react";
import * as THREE from "three";
import { OrbitControls } from "three/examples/jsm/controls/OrbitControls";
import { World } from "./World"
import { ConwayAgent,ConwayControl } from "./ConwayAgent"
import SimControl from "./SimControl"
import CameraControl from "./CameraControl"

const style = {
  height: 512,
  maxWidth: 1024
};

let delta = 0;
let clock = new THREE.Clock();
let interval = 1 / 30;

const transmissionDelayMax = 0;  // ms max

class App extends Component {
  state = {}
  debugNetwork = false;
  debugNeighbors = false;
  showNeighborsFor = -1;
  messages = [];
  simPaused = true;
  pauseTime = Number.MAX_SAFE_INTEGER;
  time = 0;  // Simulation time
  lastLoopTime = new Date().getTime();

  agentParams = {startPattern:"test", size: 10,
    cycleTime: 400,
    stateSendTime: 50,
    spontaneousGeneration: 0.001*0,
    resurrectionChance: 0.5*0};

  componentDidMount() {
    this.worldSetup();
    this.agentSetup(this.agentParams);
    this.sceneSetup();
    this.addCustomSceneObjects();
    this.startAnimationLoop();
    window.addEventListener("resize", this.handleWindowResize);

    this.setState({time:0});

    // Render once so we see the initial pattern
    this.renderer.render(this.scene, this.camera);
  }

  componentWillUnmount() {
    window.removeEventListener("resize", this.handleWindowResize);
    window.cancelAnimationFrame(this.requestID);
    this.controls.dispose();
  }

  // Setup the world environment.
  worldSetup = () => {
    this.world = new World();
  };

  // Setup the agents
  agentSetup = (params) => {
    let id = 0;

    let random = false;
    let chance = 0.4;
    let quadpole = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,1,1,0,0,0,0,0,0,0],
      [0,1,0,1,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,1,0,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,1,0,1,0,0],
      [0,0,0,0,0,0,1,1,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let blank = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let corners = [
      [1,1,0,0,0,0,0,0,1,1],
      [1,0,0,0,0,0,0,0,0,1],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [1,0,0,0,0,0,0,0,0,1],
      [1,1,0,0,0,0,0,0,1,1],
    ];
    let cool1 = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,1,1,0,0,0,0],
      [0,0,0,1,1,1,1,0,0,0],
      [0,0,0,0,1,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let toad = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,1,1,1,0,0,0],
      [0,0,0,1,1,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let blinker = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,1,0,0,0,0],
      [0,0,0,0,0,1,0,0,0,0],
      [0,0,0,0,0,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let beacon = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,1,1,0,0,0,0,0],
      [0,0,0,1,1,0,0,0,0,0],
      [0,0,0,0,0,1,1,0,0,0],
      [0,0,0,0,0,1,1,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let pulsar = [
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      [0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0],
      [0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0],
      [0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0],
      [0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0],
      [0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0],
      [0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0],
      [0,0,1,0,0,0,0,1,0,1,0,0,0,0,1,0,0],
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0],
    ];

    let test = [
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,1,1,0,0,0,0,0],
      [0,0,0,1,1,0,0,0,0,0],
      [0,0,0,0,0,1,1,0,0,0],
      [0,0,0,0,0,1,1,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
      [0,0,0,0,0,0,0,0,0,0],
    ];

    let pattern;

    if (params.startPattern === "random") {
      random = true;
    } else {
      switch(params.startPattern) {
        case "quadpole":
          pattern = quadpole;
          break;
        case "blank":
          pattern = blank;
          break;
        case "test":
          pattern = test;
          break;
        case "corners":
          pattern = corners;
          break;
        case "toad":
          pattern = toad;
          break;
        case "beacon":
          pattern = beacon;
          break;
        case "blinker":
          pattern = blinker;
          break;
        case "pulsar":
          pattern = pulsar;
          break;
        default:
          console.log("Unknown pattern");
          pattern = test;
          break;
      }
    }

    let size = params.size;
    if (size < pattern.length) {
      size = pattern.length;
    }

    this.agents = [];

    for(let y=0; y < size; y++) {
      for(let x=0; x < size; x++) {
        let alive = true;

        let px = x;
        let py = y;

        if (random) {
          alive = Math.random() < chance;
        } else {
          px = x;
          py = size - y - 1;

          alive = pattern[y][x] === 1;
        }

        const agent = new ConwayAgent(THREE,this,id++,this.world,0.1,px-size/2,py-size/2,0,Math.PI/2,0,0,params,alive);
        //console.log("agent: " + (id-1) + " id: " + (y * size + x) + " x: " + x + " y: " + y + " alive: " + alive);
        this.agents[y * size + x] = agent;
      }
    }
  };

  // Standard scene setup in Three.js. Check "Creating a scene" manual for more information
  // https://threejs.org/docs/#manual/en/introduction/Creating-a-scene
  sceneSetup = () => {
    // get container dimensions and use them for scene sizing
    const width = this.el.clientWidth;
    const height = this.el.clientHeight;

    this.scene = new THREE.Scene();
    this.camera = new THREE.PerspectiveCamera(
      75, // fov = field of view
      width / height, // aspect ratio
      0.1, // near plane
      1000.0 // far plane
    );
    this.camera.position.z = 10; // is used here to set some distance from a cube that is located at z = 0
    // OrbitControls allow a camera to orbit around the object
    // https://threejs.org/docs/#examples/controls/OrbitControls

    this.controls = new OrbitControls(this.camera, this.el);
    this.renderer = new THREE.WebGLRenderer();
    this.renderer.setSize(width, height);
    this.el.appendChild(this.renderer.domElement); // mount using React ref

  };

  // Here should come custom code.
  // Code below is taken from Three.js BoxGeometry example
  // https://threejs.org/docs/#api/en/geometries/BoxGeometry
  addCustomSceneObjects = () => {
    for(let n=0; n < this.agents.length; n++) {
      this.scene.add(this.agents[n].getMesh());
    }

    this.calcAgentNeighbors(8);

    const lights = [];
    lights[0] = new THREE.PointLight(0xffffff, 1, 0);
    lights[1] = new THREE.PointLight(0xffffff, 1, 0);
    lights[2] = new THREE.PointLight(0xffffff, 1, 0);

    lights[0].position.set(0, 200, 0);
    lights[1].position.set(100, 200, 100);
    lights[2].position.set(-100, -200, -100);

    //this.scene.add(lights[0]);
    //this.scene.add(lights[1]);
    //this.scene.add(lights[2]);
  };

  // Calculate agent neighbors based on distance.
  calcAgentNeighbors(max) {
    if (this.debugNeighbors) console.log("Calculating neighbors");

    for(let agentIdx=0; agentIdx < this.agents.length; agentIdx++) {
      if (this.debugNeighbors) console.log("Agent: " + agentIdx);
      let distances = [];

      let agent = this.agents[agentIdx];
      let pos = agent.getPosition();

      for (let i = 0; i < this.agents.length; i++) {
        if (i !== agentIdx) {
          let dist = this.agents[i].getPosition().distanceToSquared(pos);
          distances.push({dist: dist, agentIdx: i});
        }
      }

      distances.sort(function (a, b) {
        return a.dist - b.dist;
      });
      if (this.debugNeighbors) console.log(JSON.stringify(distances));

      //console.log(distances);
      let neighbors = [];
      let len = Math.min(max,distances.length);

      for(let i=0; i < len; i++) {
        neighbors[i] = distances[i].agentIdx;
      }

      if (this.debugNeighbors) console.log(JSON.stringify(neighbors));
      this.agents[agentIdx].setNeighbors(neighbors);
    }

    if (this.showNeighborsFor !== -1) {
      let neighbors = this.agents[this.showNeighborsFor].getNeighbors();
      for(let i=0; i < neighbors.length; i++) {
        this.agents[neighbors[i]].setMarked(true);
      }
    }
  }

  startAnimationLoop = () => {
    this.requestID = window.requestAnimationFrame(this.startAnimationLoop);

    let now = new Date().getTime();

    if (!this.simPaused) {
      // Handle delivery of messages
      this.time += now - this.lastLoopTime;
      this.setState({time:this.time});

      let remove = [];
      for (let i = 0; i < this.messages.length; i++) {
        if (this.messages[i].time < this.time) {
          if (this.debugNetwork) console.log("Routing message: " + JSON.stringify(this.messages[i]));
          if (typeof this.messages[i].to === 'undefined') {
            console.log("Invalid message: " + JSON.stringify(this.messages[i]))
          } else {
            this.agents[this.messages[i].to].receiveMessage(this.messages[i]);
          }
          remove.push(i);
        }
      }

      for (let i = 0; i < remove.length; i++) {
        this.messages.splice(remove[i] - i, 1);
      }

      // Handle clocking of agents
      // TODO: Likely we should randomally walk for better simulation of real world parallelism
      for (let i = 0; i < this.agents.length; i++) {
        this.agents[i].loop(this.time);
      }

      //this.sleep(50);  // TODO: Remove this after debugging
      if (this.time > this.pauseTime) {
        this.simPaused = true;
        this.renderer.render(this.scene, this.camera);
        this.pauseTime = Number.MAX_SAFE_INTEGER;
        return;
      }
    }

    this.lastLoopTime = now;
    delta += clock.getDelta();
    if (delta < interval) {
      return;
    }

    this.renderer.render(this.scene, this.camera);
    delta = delta % interval;
  };

  sleep(milliseconds) {
    const date = Date.now();
    let currentDate = null;
    do {
      currentDate = Date.now();
    } while (currentDate - date < milliseconds);
  }

  handleWindowResize = () => {
    const width = this.el.clientWidth;
    const height = this.el.clientHeight;

    this.renderer.setSize(width, height);
    this.camera.aspect = width / height;

    // Note that after making changes to most of camera properties you have to call
    // .updateProjectionMatrix for the changes to take effect.
    this.camera.updateProjectionMatrix();
  };

  sendMessage(from,to,msg) {
    if (this.debugNetwork) {
      console.log("Send message. from: " + from + " to: " + to + " msg: " + JSON.stringify(msg));
    }

    if (typeof from === "undefined") {
      throw new Error("From is required");
    }
    if (typeof to === "undefined") {
      throw new Error("To is required");
    }

    this.messages.push({from:from,to:to,msg:msg,time: this.time + Math.random() * transmissionDelayMax});
  }

  handleSimPause() {
    this.simPaused = true;
  }

  handleSimPlay() {
    this.simPaused = false;
    this.lastLoopTime = new Date().getTime();
  }

  // Play the sim for one cycle time
  handleSimStep() {
    this.simPaused = false;
    this.lastLoopTime = new Date().getTime();
    this.pauseTime = this.time + this.agentParams.cycleTime;
  }

  handleSimReset() {
    this.simPaused = true;
    this.time = 0;
    this.setState({time:this.time});
    this.lastLoopTime = new Date().getTime();
    this.messages = [];

    while(this.scene.children.length > 0){
      this.scene.remove(this.scene.children[0]);
    }

    this.agentSetup(this.agentParams);
    this.addCustomSceneObjects();
  }

  handleCameraReset() {
    this.controls.reset();
  }

  handleAgentParamsChanged(params) {
    console.log("Params changed: " + JSON.stringify(params));

    this.agentParams = {...this.agentParams,...params};

    this.handleSimReset();
  }

  markNeighbors(agentIdx) {
    // clear old
    for(let i=0; i < this.agents.length; i++) {
      this.agents[i].setMarked(false);
    }

    let neighbors = this.agents[agentIdx].getNeighbors();
    for(let i=0; i < neighbors.length; i++) {
      this.agents[neighbors[i]].setMarked(!this.agents[neighbors[i]].getMarked());
    }
  }

  showAliveCount(agentIdx) {
    console.log("Agent: " + agentIdx);
    const neighbors = this.agents[agentIdx].getNeighbors();
    let aliveCount = 0;
    for(let i=0; i < neighbors.length; i++) {
      console.log("neighbor: " + neighbors[i] + " alive: " + this.agents[neighbors[i]].isAlive());
      if (this.agents[neighbors[i]].isAlive()) {
        aliveCount++;
      }
    }
    console.log("aliveCount: " + aliveCount);

  }

  handleKeyPressed(e) {
    let row = 9;
    let size = 10;

    switch(e.key) {
      case '0':
        this.markNeighbors(row * size + 0);
        this.showAliveCount(row * size + 0);
        break;
      case '1':
        this.markNeighbors(row * size + 1);
        this.showAliveCount(row * size + 1);
        break;
      case '2':
        this.markNeighbors(row * size + 2);
        this.showAliveCount(row * size + 2);
        break;
      case '3':
        this.markNeighbors(row * size + 3);
        this.showAliveCount(row * size + 3);
        break;
      case '4':
        this.markNeighbors(row * size + 4);
        this.showAliveCount(row * size + 4);
        break;
      case '5':
        this.markNeighbors(row * size + 5);
        this.showAliveCount(row * size + 5);
        break;
      case '6':
        this.markNeighbors(row * size + 6);
        this.showAliveCount(row * size + 6);
        break;
      case '7':
        this.markNeighbors(row * size + 7);
        this.showAliveCount(row * size + 7);
        break;
      case '8':
        this.markNeighbors(row * size + 8);
        this.showAliveCount(row * size + 8);
        break;
      case '9':
        this.markNeighbors(row * size + 9);
        this.showAliveCount(row * size + 9);
        break;
      case 's':
        this.handleSimStep();
        break;
      case 'c':
        for(let i=0; i < this.agents.length; i++) {
          this.agents[i].setMarked(false);
        }
        break;
      default:
        // ignore
        break;
    }
  }

  render() {
    return <div>
      <div style={style} ref={ref => (this.el = ref)} onKeyPress = {e => this.handleKeyPressed(e) } autoFocus />
      <div style={{
        display:"flex"
      }} >
        <div>
        <SimControl time = {this.state.time} resetAction={e => this.handleSimReset()} pauseAction={e => this.handleSimPause() } playAction={e => this.handleSimPlay() } stepAction={e => this.handleSimStep() }/>
        <CameraControl resetAction={e => this.handleCameraReset()} />
        </div>
        <ConwayControl {...this.agentParams} paramsChangedAction = {(param,val) => this.handleAgentParamsChanged(param,val) } />
      </div>
    </div>
  }
}

export default memo(App);

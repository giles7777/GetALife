import Geometry from "./Geometry";
import World from "./World";
import {ConwayAgent} from "./ConwayAgent";
import * as THREE from "three";

// Canyon Creek Area at Critical
// Modeled on grid coordinates from map.  Each grid is 76 meters, starting at B1 at 0,0
class CanyonCreekWorld extends World {
  scale = 76;
  points = [
    [.75,3.5],     // 0
    [1,3.5],       // 1
    [.75,3.25],    // 2
    [1,3.25],      // 3
    [.95,2.85],    // 4
    [1.25,3],      // 5
    [1.25,2.6],    // 6
    [1.75,2.6],    // 7
    [1.5,2.25],    // 8
    [1.85,2.25],   // 9
    [1.75,2],      // 10
    [2,2],         // 11
    [2,1.75],      // 12
    [2.25,1.75],   // 13
    [2.25,1.5],    // 14
    [2.4,1.6],     // 15
    [2.6,1.1],     // 16
    [2.9,1.0],     // 17
    [2.75,0.75],   // 18
    [3.0,0.75],    // 19
    [3.0, 0.5],    // 20
    [3.25,0.75],   // 21
    [3.25,0.3],    // 22
    [3.5,0.75],    // 23
    [3.5,0.25],    // 24
    [3.75,0.75],   // 25
    [3.75,0.25],   // 26
    [4.0,0.75],    // 27
    [4.5,0.75],    // 28
    [4.5,1.0],     // 29
    [4.75,0.75],   // 30
    [4.75,1.25],   // 31
    [4.75,1.25],   // 31
  ];


  getCameraStart() {
    return 50;
  }

  getLandscape(THREE) {
    const points = this.points;
    const scale = this.scale;
    
    for(let i=0; i < points.length;i++) {
      points[i] = [points[i][0], points[i][1]];
    }

    let ret = new THREE.Group();
    ret.add(Geometry.createRiver(THREE,points,scale,-1 / scale));
    ret.add(Geometry.createLand(THREE,points,scale,1,points.length-2,2,0 / scale,3.5,0,5));
    ret.add(Geometry.createLand(THREE,points,scale,0,20,2,0 / scale,-5,0,-7));
    ret.add(Geometry.createLand(THREE,points,scale,20,30,2,0 / scale,5,0,-7));

    return ret;
  }

  getAgents(THREE,pattern,params,radio) {
    const points = this.points;
    const scale = this.scale;
    let id = 0;
    const chance = 0.4;

    let agents = [];
    const water = -0.8;
    const min = 7;
    const max = 14;
    const waterSize = 0.4;
    const landSize = 0.2;

    // water points.  interpolate 3 points along each diagonal

    for(let i=min; i < max; i++) {
      const start = points[i];
      const end = points[i+1];
      const dir = new THREE.Vector3();

      dir.subVectors(new THREE.Vector3(end[0],0,end[1]),new THREE.Vector3(start[0],0,start[1]));
      dir.normalize();

      dir.multiplyScalar(0.25);
      agents.push(new ConwayAgent(THREE,radio,id++,this,waterSize,start[0] * scale + dir.x * scale,water,start[1] * scale + dir.z * scale,0,0,0,params,Math.random() < chance));

      dir.multiplyScalar(0.5);
      agents.push(new ConwayAgent(THREE,radio,id++,this,waterSize,start[0] * scale + dir.x * scale,water,start[1] * scale + dir.z * scale,0,0,0,params,Math.random() < chance));

      dir.multiplyScalar(0.75);
      agents.push(new ConwayAgent(THREE,radio,id++,this,waterSize,start[0] * scale + dir.x * scale,water,start[1] * scale + dir.z * scale,0,0,0,params,Math.random() < chance));
    }

    // land points.  put a tree on each point and each mid point

    for(let i=min; i < max - 1; i++) {
      const start = points[i];
      const end = points[i + 2];
      const shore = new THREE.Vector3();
      const dir = new THREE.Vector3();
      let rot = 0;

      if (i % 2 === 0) {
        rot = -Math.PI / 2;
      } else {
        rot = Math.PI / 2;
      }

      shore.subVectors(new THREE.Vector3(points[i+1][0], 0, points[i+1][1]), new THREE.Vector3(start[0], 0, start[1]));
      shore.normalize();

      console.log("shore: " + shore.x + " " + shore.y + " " + shore.z);
      dir.subVectors(new THREE.Vector3(end[0], 0, end[1]), new THREE.Vector3(start[0], 0, start[1]));
      dir.normalize();

      shore.multiplyScalar(-0.05);  // Back off some into the forest
      console.log("water.  pos: " + (start[0] * scale + shore.x * scale) + " " + water + " " + (start[1] * scale + shore.z * scale));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + shore.x * scale,0,start[1] * scale + shore.y * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + shore.x * scale,1,start[1] * scale + shore.y * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + shore.x * scale,2,start[1] * scale + shore.y * scale,0,0,rot,params,Math.random() < chance));

      dir.multiplyScalar(0.25);

      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,0,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,1,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,2,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));

      dir.multiplyScalar(0.5);

      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,0,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,1,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,2,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));

      dir.multiplyScalar(0.75);

      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,0,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,1,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
      agents.push(new ConwayAgent(THREE,radio,id++,this,landSize,start[0] * scale + dir.x * scale + shore.x * scale,2,start[1] * scale + dir.z * scale + shore.z * scale,0,0,rot,params,Math.random() < chance));
    }

    return agents;
  }
  
  getViewpoints() {
    const scale = this.scale;

    const vp = [];
    vp.push({name:"Grid", position:[0,0,15]});
    vp.push({name:"Pot Luck Camp", position:[1.75 * scale,15 / scale,2.6 * scale], lookat: [1.5 * scale,2 / scale,2.25 * scale]});
    vp.push({name:"Far", position:[70,2,300]});
    vp.push({name:"Float End", position:[70,2,280]});

    return vp;
  }
}

export default CanyonCreekWorld;
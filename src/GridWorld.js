import Geometry from "./Geometry";
import World from "./World";

// Grid of size x size
class GridWorld extends World {
  constructor(size) {
    super();

    this.size = size;
  }

  getCameraStart() {
    return this.size;
  }
  
  getLandscape(THREE) {
    return new THREE.Group();
  }
}

export default GridWorld;
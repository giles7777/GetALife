// World Environmental State.  Provides values based on x,y,z location in meters
export class World {
  constructor() {

  }

  getTemperature(x,y,z) {
    return 70;
  }

  getHumidity(x,y,z) {
    return 70;
  }

  getLandscape() {
    return null;
  }

  // Get the camera location to start at
  getCameraStart() {
    return 5;
  }

  // Get the viewpoints
  getViewpoints() {
    return [];
  }

  getAgents() {
    return [];
  }
}

export default World;
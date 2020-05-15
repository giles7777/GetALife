class Geometry {
  // Create a river via shore points
  static createRiver(THREE,points,scale,y) {
    let geometry = new THREE.Geometry();

    for(let i=0; i < points.length; i++) {
      //console.log(points[i][0] + " 0 " + points[i][1]);
      geometry.vertices.push(
        new THREE.Vector3(scale * points[i][0], scale * y, scale * points[i][1]),
      );
    }

    for(let i=0; i < points.length / 2; i++) {
        //console.log("eface: " + (i*2) + "," + ((i*2)+1) + "," + ((i*2)+2));
        //console.log("oface: " + (i*2+1) + "," + ((i*2)+3) + "," + (i*2+2));

        geometry.faces.push(new THREE.Face3(i*2,(i*2)+1,(i*2)+2));
        geometry.faces.push(new THREE.Face3(i*2+1,i*2+3,i*2+2));
    }

    //geometry.computeBoundingSphere();

    let material = new THREE.MeshBasicMaterial({
      color: 0x0077be,
      side: THREE.DoubleSide,
      opacity: 0.5,
      transparent: true
    });
    let mesh = new THREE.Mesh(geometry, material);

    return mesh;
  }

  // Create land via shore points.  Forms a fan from the points and origin
  static createLand(THREE,points,scale,min,max,stride,y,x0,y0,z0) {
    let geometry = new THREE.Geometry();

    geometry.vertices.push(
      new THREE.Vector3(x0 * scale, y0 * scale, z0 * scale),
    );

    for(let i=min; i <= max; i = i + stride) {
      //console.log(points[i][0] + " 0 " + points[i][1]);
      geometry.vertices.push(
        new THREE.Vector3(scale * points[i][0], 0, scale * points[i][1]),
      );
    }

    const faces = (max - min) / stride;
    //console.log("Creating land faces: " + faces);
    for(let i=0; i < faces; i++) {
      //console.log("eface: " + (i*2) + "," + ((i*2)+1) + "," + ((i*2)+2));
      //console.log("oface: " + (i*2+1) + "," + ((i*2)+3) + "," + (i*2+2));
      //console.log("face: " + "0," + "," + (i+2) + "," + (i+1));
      geometry.faces.push(new THREE.Face3(0,i+2,i+1));
    }

    let material = new THREE.MeshBasicMaterial({
      color: 0x90ee90,
      side: THREE.DoubleSide
    });
    let mesh = new THREE.Mesh(geometry, material);

    return mesh;
  }

  // Create land via shore points.  Forms a fan from the points and origin
  static createLandDebug(THREE,points,scale,min,max,stride,y,x0,y0,z0) {
    let verts = [];

    verts.push(
      new THREE.Vector3(x0 * scale, y0 * scale, z0 * scale),
    );

    for(let i=min; i <= max; i = i + stride) {
      //console.log(points[i][0] + " 0 " + points[i][1]);
      verts.push(
        new THREE.Vector3(scale * points[i][0], 0, scale * points[i][1]),
      );
    }
    verts.push(
      new THREE.Vector3(x0 * scale, y0 * scale, z0 * scale),
    );

    console.log("verts: " + verts.length);
    var geometry = new THREE.BufferGeometry().setFromPoints( verts );
    var material = new THREE.LineBasicMaterial( { color: 0x00FF00 } );

    let pointMaterial = new THREE.PointsMaterial({
      size: 1,
      color: 0xFF0000
    });

    var lineset = new THREE.Line( geometry, material );
    var pointset = new THREE.Points(geometry, pointMaterial);

    let group = new THREE.Group();
    group.add(lineset);
    group.add(pointset);
    group.add(Geometry.createLand(THREE,points,scale,min,max,stride,y,x0,y0,z0));

    // Create debug labels
    for(let i=0; i < verts.length; i++) {
      const canvas = Geometry.makeLabelCanvas(128, 64, i);
      const texture = new THREE.CanvasTexture(canvas);
      // because our canvas is likely not a power of 2
      // in both dimensions set the filtering appropriately.
      texture.minFilter = THREE.LinearFilter;
      texture.wrapS = THREE.ClampToEdgeWrapping;
      texture.wrapT = THREE.ClampToEdgeWrapping;

      const labelMaterial = new THREE.SpriteMaterial({
        map: texture,
        transparent: true,
      });

      const label = new THREE.Sprite(labelMaterial);
      label.position.x = verts[i].x;
      label.position.y = (y + 0.01) * scale;
      label.position.z = verts[i].z;

      // if units are meters then 0.01 here makes size
      // of the label into centimeters.
      const labelBaseScale = 0.001;
      label.scale.x = 1060 * labelBaseScale;  // TODO: Do not hardcode these
      label.scale.y = 512 * labelBaseScale;

      group.add(label);
    }

    return group;

  }

  static createRiverDebug(THREE,points,scale,y) {
    let verts = [];
    for(let i=0; i < points.length; i++) {
      //console.log(points[i][0] + " 0 " + points[i][1]);
      verts.push(
        new THREE.Vector3(scale*points[i][0], scale * y, scale*points[i][1]),
      );
    }

    var geometry = new THREE.BufferGeometry().setFromPoints( verts );
    var material = new THREE.LineBasicMaterial( { color: 0x00FF00 } );

    let pointMaterial = new THREE.PointsMaterial({
      size: 1,
      color: 0xFF0000
    });

    var lineset = new THREE.Line( geometry, material );
    var pointset = new THREE.Points(geometry, pointMaterial);

    let group = new THREE.Group();
    group.add(lineset);
    group.add(pointset);
    group.add(Geometry.createRiver(THREE,points,scale,y));

    // Create debug labels
    for(let i=0; i < points.length; i++) {
      const canvas = Geometry.makeLabelCanvas(128, 64, i);
      const texture = new THREE.CanvasTexture(canvas);
      // because our canvas is likely not a power of 2
      // in both dimensions set the filtering appropriately.
      texture.minFilter = THREE.LinearFilter;
      texture.wrapS = THREE.ClampToEdgeWrapping;
      texture.wrapT = THREE.ClampToEdgeWrapping;

      const labelMaterial = new THREE.SpriteMaterial({
        map: texture,
        transparent: true,
      });

      const label = new THREE.Sprite(labelMaterial);
      label.position.x = scale * points[i][0];
      label.position.y = (y + 0.01) * scale;
      label.position.z = scale * points[i][1];

      // if units are meters then 0.01 here makes size
      // of the label into centimeters.
      const labelBaseScale = 0.001;
      label.scale.x = 1060 * labelBaseScale;  // TODO: Do not hardcode these
      label.scale.y = 512 * labelBaseScale;

      group.add(label);
    }

    return group;
  }

  static createBox(THREE,points) {
    let geometry = new THREE.Geometry();

    geometry.vertices.push(
      new THREE.Vector3(-1, -1,  1),  // 0
      new THREE.Vector3( 1, -1,  1),  // 1
      new THREE.Vector3(-1,  1,  1),  // 2
      new THREE.Vector3( 1,  1,  1),  // 3
      new THREE.Vector3(-1, -1, -1),  // 4
      new THREE.Vector3( 1, -1, -1),  // 5
      new THREE.Vector3(-1,  1, -1),  // 6
      new THREE.Vector3( 1,  1, -1),  // 7
    );

    geometry.faces.push(
      // front
      new THREE.Face3(0, 3, 2),
      new THREE.Face3(0, 1, 3),
      // right
      new THREE.Face3(1, 7, 3),
      new THREE.Face3(1, 5, 7),
      // back
      new THREE.Face3(5, 6, 7),
      new THREE.Face3(5, 4, 6),
      // left
      new THREE.Face3(4, 2, 6),
      new THREE.Face3(4, 0, 2),
      // top
      new THREE.Face3(2, 7, 6),
      new THREE.Face3(2, 3, 7),
      // bottom
      new THREE.Face3(4, 1, 0),
      new THREE.Face3(4, 5, 1),
    );
    //geometry.computeBoundingSphere();

    let material = new THREE.MeshBasicMaterial({
      color: 0xFFFFFF,
      side: THREE.DoubleSide
    });
    let mesh = new THREE.Mesh(geometry, material);

    return mesh;
  }

  static makeLabelCanvas(baseWidth, size, name) {
    const borderSize = 2;
    const ctx = document.createElement('canvas').getContext('2d');
    const font =  `${size}px bold sans-serif`;
    ctx.font = font;
    // measure how long the name will be
    const textWidth = ctx.measureText(name).width;

    const doubleBorderSize = borderSize * 2;
    const width = baseWidth + doubleBorderSize;
    const height = size + doubleBorderSize;
    ctx.canvas.width = width;
    ctx.canvas.height = height;

    // need to set font again after resizing canvas
    ctx.font = font;
    ctx.textBaseline = 'middle';
    ctx.textAlign = 'center';

    ctx.fillStyle = 'blue';
    ctx.fillRect(0, 0, width, height);

    // scale to fit but don't stretch
    const scaleFactor = Math.min(1, baseWidth / textWidth);
    ctx.translate(width / 2, height / 2);
    ctx.scale(scaleFactor, 1);
    ctx.fillStyle = 'white';
    ctx.fillText(name, 0, 0);

    return ctx.canvas;
  }


}

export default Geometry
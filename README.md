# GetALife - Mesh Networked Artificial Life Forms

## Art Project 

### Overview

GetALife proposes to bring the forests and waterways of the pacific northwest alive with playful curiosity.  In homage to Conways Game of Life these creatures evolve based
on their environment and participant interactions.

Creatures of different species will inhabit our world.  Some will be water tight and reside in the water.  Others will cling to the sides of trees or be worn by humans.  
Each will have its own set of hardware and behavior.  

An initial prototype is available here: 
<a href="https://giles7777.github.io/giles7777.github.io/index.html" target="_blank">GetALife Prototype</a>

### Philosophical Statement

TBD

## Hardware Environment

GetALife is based on a distributed microcontroller (uC) environment that uses mesh netowrking.  The uC's are likely ESP8266-based, generally, which provides a convenient WiFi stack under-the-hood.  Nodes will be meshed networked with no central WIFI router.

The uC's have hardware-level connections to peripheral devices that fall into two categories: sensors and actuators.  

### Sensors

Sensors provide input to the system for the purposes of interactivity.  These might be light levels, temperature, humidity.  


### Actuators

Actuators provide output from the system to generate actions, behaviors and gestures.  These might be LED's, sound, fog, water pumps.


## Software Environment

GetALife is programmed in C++ within the Arduino IDE.  It is impossible to cleanly delineate where the Electronics and Software functional domains form an interface.  
The general idea is that the Electronics group delivers a set of working hardware with a minimal code base that exercises the hardware.  
While it is possible to reprogram the uC's in-the-field, remember that these systems could be installed in the middle of a river in a watertight enclosure.  
There will no global controller for this project, each node is expected to respond to others.  Ideally all node types can use the same codebase with no per-node configuration.  




-----
This project was bootstrapped with [Create React App](https://github.com/facebook/create-react-app).  To start developing you'll need to install the following pieces:
* NPM - https://www.npmjs.com/get-npm
* Yarn - https://classic.yarnpkg.com/en/docs/install/#windows-stable

## Available Scripts

In the project directory, you can run:

### `yarn start`

Runs the app in the development mode.<br />
Open [http://localhost:3000](http://localhost:3000) to view it in the browser.

The page will reload if you make edits.<br />
You will also see any lint errors in the console.

### `yarn build`

Builds the app for production to the `build` folder.<br />
It correctly bundles React in production mode and optimizes the build for the best performance.

The build is minified and the filenames include the hashes.<br />
Your app is ready to be deployed!

See the section about [deployment](https://facebook.github.io/create-react-app/docs/deployment) for more information.



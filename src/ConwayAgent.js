import React, { useState } from "react";

import {Agent} from "./Agent";
import {Button} from "@rmwc/button";
import {TextField} from "@rmwc/textfield"
import {Select} from "@rmwc/select"

import '@material/textfield/dist/mdc.textfield.css';
import '@rmwc/select/select.css';
import '@material/select/dist/mdc.select.css';
import '@material/floating-label/dist/mdc.floating-label.css';
import '@material/notched-outline/dist/mdc.notched-outline.css';
import '@material/line-ripple/dist/mdc.line-ripple.css';
import '@material/list/dist/mdc.list.css';
import '@material/menu/dist/mdc.menu.css';
import '@material/menu-surface/dist/mdc.menu-surface.css';
import '@material/ripple/dist/mdc.ripple.css';

const States = {Idle:"Idle",Sensing:"Sensing"};
const Messages = {AreYouAlive:"AreYouAlive",ImAlive:"ImAlive",ImDead:"ImDead",Resurrect:"Resurrect"};


/*
Agent based on Conway's Game of Life
 */
export class ConwayAgent extends Agent {
  debugSim = false;
  debugNetwork = false;
  lastStateSend = 0;
  lastCycle = 0;
  neighborState = new Map();

  constructor(context,radio,id,world,size,x,y,z,rotx=0,roty=0,rotz=0,params,alive) {
    super(context,radio,id,world,size,x,y,z,rotx,roty,rotz);

    this.params = params;
    this.setAlive(alive);
    this.state.lastCycle = 0;
    this.state.state = States.Idle;
    this.state.batch = 0;
    this.state.aliveCount = 0;
  }

  setAlive(alive) {
    this.state.alive = alive;

    if (alive) {
//      this.material.color.setHex(0x0000FF);
      this.material.color.setHex(0xFF00FF);
    } else {
      this.material.color.setHex(0x222222);
    }
  }

  isAlive() {
    return this.state.alive;
  }

  loop(time) {
    if (time > this.lastStateSend + this.params.stateSendTime) {
      this.lastStateSend = time;
      let msg = {command:this.state.alive ?Messages.ImAlive:Messages.ImDead};
      if (this.debugSim && this.id === 0) {
        console.log("Sending state: " + this.state.alive + " to neighbors: " + this.neighbors.length);
        //console.log(JSON.stringify(msg));
      }
      for(let i=0; i < this.neighbors.length; i++) {
        this.radio.sendMessage(this.id, this.neighbors[i], msg);
      }
    }

    if (this.debugSim && this.id === 0) console.log("Check cycle.  time: " + time + " next: " + (this.lastCycle + this.params.cycleTime) + " lastCycle: " + this.lastCycle + " cycleTime: " + this.params.cycleTime);
    if (time > this.lastCycle + this.params.cycleTime) {
      if (this.debugSim && this.id === 0) {
        console.log("Cycle done.  Updating state.  neighbors: " + this.neighborState.size);
      }
      this.lastCycle = time;
      let aliveCount = 0;
      this.neighborState.forEach( (k,v) => {
        if (this.debugSim && this.id === 0) console.log(k, v);
        if (k) {
          aliveCount++;
        }
      } );
      /*
      for(let [k,v] of this.neighborState) {
        if (v === true) aliveCount++;
        if (this.debugSim && this.id === 0) {
          //console.log("Agent: " + k + " alive: " + v);
        }
      }
       */

      if (this.debugSim && this.id === 0) {
        console.log("Agent: " + this.id + " aliveN: " + aliveCount);
      }
      if (this.state.alive === true && (aliveCount === 2 || aliveCount === 3)) {
        this.setAlive(true);
      } else if (this.state.alive === false) {

        if (aliveCount === 3) {
          this.setAlive(true);
        } else {
          if (Math.random() < this.params.spontaneousGeneration) {
            //console.log("Spontanous Generate");
            this.setAlive(true);
            if (Math.random() < this.params.resurrectionChance) {
              //console.log("Awake my brothers!");
              // Resurrect your neighbors
              for(let i=0; i < this.neighbors.length; i++) {
                let msg = {command:Messages.Resurrect};
                this.radio.sendMessage(this.id, this.neighbors[i], msg);
              }
            }
          }
        }
      } else {
        this.setAlive(false);
      }
    }
  }

  // Receive a network message.  Contains from,to,msg properties
  receiveMessage(wrapper) {
    if (this.debugNetwork && this.id === 0) {
      console.log("Agent: " + this.id + " recv: " + JSON.stringify(wrapper));
    }

    let msg = wrapper.msg;

    switch(msg.command) {
      case Messages.Resurrect:
        let r = Math.random();
        //console.log("Got resurrect.  rolled: " + r + " vs: " + this.params.resurrectionChance);
        if (r < this.params.resurrectionChance) {
          //console.log("Jesus!");
          this.setAlive(true);
        }
        break;
      case Messages.ImAlive:
        this.neighborState.set(wrapper.from,true);
        break;
      case Messages.ImDead:
        this.neighborState.set(wrapper.from,false);
        break;
      default:
        // ignore
        break;
    }
  }

  /*
    // Simulation methods
    loop(time) {
      if (this.state.state === States.Idle) {
        //if (this.id === 0) console.log("time: " + time + " next: " + (this.state.lastCycle + this.params.cycleTime));
        if (time < this.state.lastCycle + this.params.cycleTime) return;

        if (this.debugSim && this.id == 0) {
          console.log("Start Sensing: " + time);
        }

        this.state.state = States.Sensing;
        this.state.startedSensing = time;
        this.state.batch = this.state.batch + 1;
        this.state.aliveCount = 0;

        // Query our neighbors.
        for(let i=0; i < this.neighbors.length; i++) {
          let msg = {command:Messages.AreYouAlive, batch:this.state.batch};
          this.radio.sendMessage(this.id, this.neighbors[i], msg);
        }
      } else if (this.state.state === States.Sensing) {
        if (time > this.state.startedSensing + this.params.cycleTime) {
          if (this.debugSim && this.id == 0) {
            console.log("End Sensing: " + time)
          }
          // We've collected as many answers as we are going to get
          if (this.state.alive === true && (this.state.aliveCount === 2 || this.state.aliveCount === 3)) {
            this.setAlive(true);
          } else if (this.state.alive === false) {

            if (this.state.aliveCount === 3) {
              this.setAlive(true);
            } else {
              if (Math.random() < this.params.spontaneousGeneration) {
                //console.log("Spontanous Generate");
                this.setAlive(true);
                if (Math.random() < this.params.resurrectionChance) {
                  //console.log("Awake my brothers!");
                  // Resurrect your neighbors
                  for(let i=0; i < this.neighbors.length; i++) {
                    let msg = {command:Messages.Resurrect};
                    this.radio.sendMessage(this.id, this.neighbors[i], msg);
                  }
                }
              }
            }
          } else {
            this.setAlive(false);
          }

          this.state.state = States.Idle;
          this.state.lastCycle = time;
          return;
        }
      }
    }

    // Receive a network message.  Contains from,to,msg properties
    receiveMessage(wrapper) {
      if (this.debugNetwork) {
        console.log("Agent: " + this.id + " recv: " + JSON.stringify(wrapper));
      }

      let msg = wrapper.msg;

      if (msg.command === Messages.Resurrect) {
        let r = Math.random();
        //console.log("Got resurrect.  rolled: " + r + " vs: " + this.params.resurrectionChance);
        if (r < this.params.resurrectionChance) {
          //console.log("Jesus!");
          this.setAlive(true);
        }
      }

      if (this.state.state !== States.Sensing) {
        if (this.debugNetwork) {
          console.log("Agent: " + this.id + " ignored.  Not Sensing");
        }
        // ignore
        return;
      }

      switch(msg.command) {
        case Messages.ImAlive:
          if (msg.batch !== this.state.batch) {
            // ignore old batch answer
            return;
          }

          this.state.aliveCount++;
          break;
        case Messages.AreYouAlive:
          if (this.state.alive === true) {
            this.radio.sendMessage(this.id, wrapper.from, {command: Messages.ImAlive, batch: msg.batch})
          }
          // Do not send a message if dead to preserve bandwidth
          break;
      }
    }
  */

  render() {
    return <div><h1>Game of Life</h1></div>
  }
}

export function ConwayControl(props) {
  const [startPattern,setStartPattern] = useState(props.startPattern);
  const [size,setSize] = useState(props.size);
  const [cycleTime, setCycleTime] = useState(props.cycleTime);
  const [spontaneousGeneration, setSpontaneousGeneration] = useState(props.spontaneousGeneration);
  const [resurrectionChance,setResurrectionChance] = useState(props.resurrectionChance);

  const submit = (evt) => {
    if (evt) evt.preventDefault();

    props.paramsChangedAction({
      startPattern:startPattern,
      size:size,
      cycleTime:cycleTime,
      spontaneousGeneration:spontaneousGeneration,
      resurrectionChance:resurrectionChance
    });

    return null;
  };

  return (
    <div style={{
      width:"512px",
      border: "1px solid black",
      padding:"8px"
    }}>
      <h1>Game of Life - Agent Control</h1>
      <form onSubmit={submit}>
        <Select style={{
          width:"100%",
          height:"56px",
          margin:"auto",
          display:"block",
          marginTop:"24px",
        }}
          label="Start Pattern" options={["critical","toad","beacon","blinker","random","corners","blank","quadpole","pulsar","test"]} value={startPattern} onChange={e => setStartPattern(e.target.value)} />
        <TextField style={{
          width:"100%",
          height:"56px",
          margin:"auto",
          display:"block",
          marginTop:"24px",
        }}label="Size" required value={size} onChange={e => setSize(parseInt(e.target.value))} />
        <TextField style={{
          width:"100%",
          height:"56px",
          margin:"auto",
          display:"block",
          marginTop:"24px",
        }}label="CycleTime" required value={cycleTime} onChange={e => setCycleTime(parseInt(e.target.value))} />
        <TextField style={{
          width:"100%",
          height:"56px",
          margin:"auto",
          display:"block",
          marginTop:"24px",
        }}label="Spontaneous Generation" required value={spontaneousGeneration} onChange={e => setSpontaneousGeneration(e.target.value)} />
        <TextField style={{
          width:"100%",
          height:"56px",
          margin:"auto",
          display:"block",
          marginTop:"24px",
        }}label="Resurrection Chance" required value={resurrectionChance} onChange={e => setResurrectionChance(e.target.value)} />
        <Button style={{
          width:"100%"
        }} type="submit" label="Update" />
      </form>
    </div>
  );
}
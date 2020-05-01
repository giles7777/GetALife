import React, { memo } from "react";

import { Button } from "@rmwc/button"

import '@material/button/dist/mdc.button.css';
import '@material/ripple/dist/mdc.ripple.css';

/*
 Simulation Control Component

  props:
     resetAction
     pauseAction
     playAction
 */
function SimControl(props) {
  return (
    <div style={{
      border: "1px solid black",
      padding:"8px"
    }}>
      <h1>Simulation Control</h1>
      <div style={{width:"100px",margin:"auto"}}><b>time: {props.time}</b></div>
      <Button label="Play" onClick={props.playAction} />
      <Button label="Pause" onClick={props.pauseAction}/>
      <Button label="Step" onClick={props.stepAction}/>
      <Button label="Reset" onClick={props.resetAction} />
    </div>
  );
}

export default memo(SimControl);

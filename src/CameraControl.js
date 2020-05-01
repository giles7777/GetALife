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
function CameraControl(props) {
  return (
    <div style={{
      border: "1px solid black",
      padding:"8px"
    }}>
      <h1>Camera Control</h1>
      <Button label="Reset" onClick={props.resetAction} />
    </div>
  );
}

export default memo(CameraControl);

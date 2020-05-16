import React, {memo, useEffect, useState} from "react";

import {Button} from "@rmwc/button"

import '@material/button/dist/mdc.button.css';
import '@material/ripple/dist/mdc.ripple.css';
import {Select} from "@rmwc/select";

/*
 Simulation Control Component

  props:
     resetAction
     vp
 */
function CameraControl(props) {
  const [viewpoint, setViewpoint] = useState("");
  const [viewpointNames, setViewpointNames] = useState([]);
  const [navMode, setNavMode] = useState("Orbit");

  useEffect(
    () => {
      let vps = [];
      for (let i = 0; i < props.viewpoints.length; i++) {
        vps.push(props.viewpoints[i].name);
      }

      setViewpoint(vps[0]);
      setViewpointNames(vps);
    },
    [props.viewpoints]
  );


  function submit(evt) {
    if (evt) evt.preventDefault();

    for(let i=0; i < props.viewpoints.length; i++) {
      console.log(props.viewpoints[i]);
      if (props.viewpoints[i].name === viewpoint) {
        props.changeViewpointAction(props.viewpoints[i]);
      }
    }

    props.changeNavModeAction(navMode);

    return null;
  }

  /*

        <Select style={{
          width: "100%",
          height: "56px",
          margin: "auto",
          display: "block",
          marginTop: "24px",
        }}
                label="NavMode" options={["Orbit","Fly"]} value={navMode}
                onChange={e => setNavMode(e.target.value)}/>


   */
  return (
    <div style={{
      border: "1px solid black",
      padding: "8px"
    }}>
      <h1>Camera Control</h1>
      <form onSubmit={submit}>
        <Select style={{
          width: "100%",
          height: "56px",
          margin: "auto",
          display: "block",
          marginTop: "24px",
        }}
                label="Viewpoint" options={viewpointNames} value={viewpoint}
                onChange={e => setViewpoint(e.target.value)}/>
          <Button style={{width:"50%"}} type="submit" label="Update"/>
          <Button style={{width:"50%"}} label="Reset" onClick={props.resetAction}/>
      </form>
    </div>
  );
}

export default memo(CameraControl);

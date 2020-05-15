import React, {useState,useEffect,memo} from "react";
import {Select} from "@rmwc/select";
import {TextField} from "@rmwc/textfield";
import {Button} from "@rmwc/button";

function WorldControl(props) {
  const [layout,setLayout] = useState(props.layout);

  const submit = (evt) => {
    if (evt) evt.preventDefault();

    props.paramsChangedAction({
      layout:layout
    });

    return null;
  };

  return (
    <div style={{
      width:"512px",
      border: "1px solid black",
      padding:"8px"
    }}>
      <h1>World Control</h1>
      <form onSubmit={submit}>
        <Select style={{
          width:"100%",
          height:"56px",
          margin:"auto",
          display:"block",
          marginTop:"24px",
        }}
                label="Layout" options={["grid","critical","forest"]} value={layout} onChange={e => setLayout(e.target.value)} />
        <Button style={{
          width:"100%"
        }} type="submit" label="Update" />
      </form>
    </div>
  );
}

export default memo(WorldControl);
export default () => {
  // Handles agent to agent messages
  self.addEventListener('message', e => { // eslint-disable-line no-restricted-globals
    if (!e) return;

    postMessage({r:1,g:1,b:1})
  });

  // Handles agent level sensor
  setInterval(() => {
  }
  ,100)
}

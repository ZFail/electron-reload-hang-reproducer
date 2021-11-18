const { createTSFN } = require('bindings')('addon');
const remote = require('@electron/remote');

const callback = (id) => { 
    console.log(id); 
    if (id >= 20) {
        remote.getCurrentWindow().reload()
        // remote.app.exit(0)
    }
};
// createTSFN(callback);

setTimeout(() => {
    createTSFN(callback);
}, 0)

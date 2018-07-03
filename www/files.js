
var Module;

if (typeof Module === 'undefined') Module = eval('(function() { try { return Module || {} } catch(e) { return {} } })()');

if (!Module.expectedDataFileDownloads) {
  Module.expectedDataFileDownloads = 0;
  Module.finishedDataFileDownloads = 0;
}
Module.expectedDataFileDownloads++;
(function() {
 var loadPackage = function(metadata) {

    var PACKAGE_PATH;
    if (typeof window === 'object') {
      PACKAGE_PATH = window['encodeURIComponent'](window.location.pathname.toString().substring(0, window.location.pathname.toString().lastIndexOf('/')) + '/');
    } else if (typeof location !== 'undefined') {
      // worker
      PACKAGE_PATH = encodeURIComponent(location.pathname.toString().substring(0, location.pathname.toString().lastIndexOf('/')) + '/');
    } else {
      throw 'using preloaded data can only be done on a web page or in a web worker';
    }
    var PACKAGE_NAME = 'icesl-online/www/files.data';
    var REMOTE_PACKAGE_BASE = 'files.data';
    if (typeof Module['locateFilePackage'] === 'function' && !Module['locateFile']) {
      Module['locateFile'] = Module['locateFilePackage'];
      Module.printErr('warning: you defined Module.locateFilePackage, that has been renamed to Module.locateFile (using your locateFilePackage for now)');
    }
    var REMOTE_PACKAGE_NAME = typeof Module['locateFile'] === 'function' ?
                              Module['locateFile'](REMOTE_PACKAGE_BASE) :
                              ((Module['filePackagePrefixURL'] || '') + REMOTE_PACKAGE_BASE);
  
    var REMOTE_PACKAGE_SIZE = metadata.remote_package_size;
    var PACKAGE_UUID = metadata.package_uuid;
  
    function fetchRemotePackage(packageName, packageSize, callback, errback) {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', packageName, true);
      xhr.responseType = 'arraybuffer';
      xhr.onprogress = function(event) {
        var url = packageName;
        var size = packageSize;
        if (event.total) size = event.total;
        if (event.loaded) {
          if (!xhr.addedTotal) {
            xhr.addedTotal = true;
            if (!Module.dataFileDownloads) Module.dataFileDownloads = {};
            Module.dataFileDownloads[url] = {
              loaded: event.loaded,
              total: size
            };
          } else {
            Module.dataFileDownloads[url].loaded = event.loaded;
          }
          var total = 0;
          var loaded = 0;
          var num = 0;
          for (var download in Module.dataFileDownloads) {
          var data = Module.dataFileDownloads[download];
            total += data.total;
            loaded += data.loaded;
            num++;
          }
          total = Math.ceil(total * Module.expectedDataFileDownloads/num);
          if (Module['setStatus']) Module['setStatus']('Downloading data... (' + loaded + '/' + total + ')');
        } else if (!Module.dataFileDownloads) {
          if (Module['setStatus']) Module['setStatus']('Downloading data...');
        }
      };
      xhr.onload = function(event) {
        var packageData = xhr.response;
        callback(packageData);
      };
      xhr.send(null);
    };

    function handleError(error) {
      console.error('package error:', error);
    };
  
      var fetched = null, fetchedCallback = null;
      fetchRemotePackage(REMOTE_PACKAGE_NAME, REMOTE_PACKAGE_SIZE, function(data) {
        if (fetchedCallback) {
          fetchedCallback(data);
          fetchedCallback = null;
        } else {
          fetched = data;
        }
      }, handleError);
    
  function runWithFS() {

    function assert(check, msg) {
      if (!check) throw msg + new Error().stack;
    }
Module['FS_createPath']('/', 'gcode', true, true);
Module['FS_createPath']('/gcode', 'ultimaker2', true, true);
Module['FS_createPath']('/gcode', 'ultimaker1', true, true);
Module['FS_createPath']('/gcode', 'reprap', true, true);
Module['FS_createPath']('/gcode', 'prusai3', true, true);
Module['FS_createPath']('/gcode', 'pharaoh_xd', true, true);
Module['FS_createPath']('/gcode', 'replicator2', true, true);
Module['FS_createPath']('/gcode', 'replicator2x', true, true);
Module['FS_createPath']('/gcode', 'replicator1_gpx', true, true);
Module['FS_createPath']('/', 'defaults', true, true);

    function DataRequest(start, end, crunched, audio) {
      this.start = start;
      this.end = end;
      this.crunched = crunched;
      this.audio = audio;
    }
    DataRequest.prototype = {
      requests: {},
      open: function(mode, name) {
        this.name = name;
        this.requests[name] = this;
        Module['addRunDependency']('fp ' + this.name);
      },
      send: function() {},
      onload: function() {
        var byteArray = this.byteArray.subarray(this.start, this.end);

          this.finish(byteArray);

      },
      finish: function(byteArray) {
        var that = this;

        Module['FS_createDataFile'](this.name, null, byteArray, true, true, true); // canOwn this data in the filesystem, it is a slide into the heap that will never change
        Module['removeRunDependency']('fp ' + that.name);

        this.requests[this.name] = null;
      },
    };

        var files = metadata.files;
        for (i = 0; i < files.length; ++i) {
          new DataRequest(files[i].start, files[i].end, files[i].crunched, files[i].audio).open('GET', files[i].filename);
        }

  
    function processPackageData(arrayBuffer) {
      Module.finishedDataFileDownloads++;
      assert(arrayBuffer, 'Loading data file failed.');
      assert(arrayBuffer instanceof ArrayBuffer, 'bad input to processPackageData');
      var byteArray = new Uint8Array(arrayBuffer);
      var curr;
      
        // copy the entire loaded file into a spot in the heap. Files will refer to slices in that. They cannot be freed though
        // (we may be allocating before malloc is ready, during startup).
        if (Module['SPLIT_MEMORY']) Module.printErr('warning: you should run the file packager with --no-heap-copy when SPLIT_MEMORY is used, otherwise copying into the heap may fail due to the splitting');
        var ptr = Module['getMemory'](byteArray.length);
        Module['HEAPU8'].set(byteArray, ptr);
        DataRequest.prototype.byteArray = Module['HEAPU8'].subarray(ptr, ptr+byteArray.length);
  
          var files = metadata.files;
          for (i = 0; i < files.length; ++i) {
            DataRequest.prototype.requests[files[i].filename].onload();
          }
              Module['removeRunDependency']('datafile_icesl-online/www/files.data');

    };
    Module['addRunDependency']('datafile_icesl-online/www/files.data');
  
    if (!Module.preloadResults) Module.preloadResults = {};
  
      Module.preloadResults[PACKAGE_NAME] = {fromCache: false};
      if (fetched) {
        processPackageData(fetched);
        fetched = null;
      } else {
        fetchedCallback = processPackageData;
      }
    
  }
  if (Module['calledRun']) {
    runWithFS();
  } else {
    if (!Module['preRun']) Module['preRun'] = [];
    Module["preRun"].push(runWithFS); // FS is not initialized yet, wait for it
  }

 }
 loadPackage({"files": [{"audio": 0, "start": 0, "crunched": 0, "end": 450, "filename": "/script.lua"}, {"audio": 0, "start": 450, "crunched": 0, "end": 2635, "filename": "/gcode/ultimaker2/printer.lua"}, {"audio": 0, "start": 2635, "crunched": 0, "end": 2734, "filename": "/gcode/ultimaker2/features.lua"}, {"audio": 0, "start": 2734, "crunched": 0, "end": 5975, "filename": "/gcode/ultimaker1/printer.lua"}, {"audio": 0, "start": 5975, "crunched": 0, "end": 6266, "filename": "/gcode/ultimaker1/features.lua"}, {"audio": 0, "start": 6266, "crunched": 0, "end": 6455, "filename": "/gcode/ultimaker1/header_1.gcode"}, {"audio": 0, "start": 6455, "crunched": 0, "end": 6679, "filename": "/gcode/ultimaker1/header_2.gcode"}, {"audio": 0, "start": 6679, "crunched": 0, "end": 8478, "filename": "/gcode/reprap/printer.lua"}, {"audio": 0, "start": 8478, "crunched": 0, "end": 8570, "filename": "/gcode/reprap/features.lua"}, {"audio": 0, "start": 8570, "crunched": 0, "end": 8891, "filename": "/gcode/reprap/header.gcode"}, {"audio": 0, "start": 8891, "crunched": 0, "end": 9015, "filename": "/gcode/reprap/footer.gcode"}, {"audio": 0, "start": 9015, "crunched": 0, "end": 10754, "filename": "/gcode/prusai3/printer.lua"}, {"audio": 0, "start": 10754, "crunched": 0, "end": 10919, "filename": "/gcode/prusai3/features.lua"}, {"audio": 0, "start": 10919, "crunched": 0, "end": 11259, "filename": "/gcode/prusai3/header.gcode"}, {"audio": 0, "start": 11259, "crunched": 0, "end": 11355, "filename": "/gcode/prusai3/footer.gcode"}, {"audio": 0, "start": 11355, "crunched": 0, "end": 13199, "filename": "/gcode/pharaoh_xd/printer.lua"}, {"audio": 0, "start": 13199, "crunched": 0, "end": 13388, "filename": "/gcode/pharaoh_xd/features.lua"}, {"audio": 0, "start": 13388, "crunched": 0, "end": 13600, "filename": "/gcode/pharaoh_xd/header.gcode"}, {"audio": 0, "start": 13600, "crunched": 0, "end": 13699, "filename": "/gcode/pharaoh_xd/footer.gcode"}, {"audio": 0, "start": 13699, "crunched": 0, "end": 15582, "filename": "/gcode/replicator2/printer.lua"}, {"audio": 0, "start": 15582, "crunched": 0, "end": 15656, "filename": "/gcode/replicator2/features.lua"}, {"audio": 0, "start": 15656, "crunched": 0, "end": 16622, "filename": "/gcode/replicator2/header.gcode"}, {"audio": 0, "start": 16622, "crunched": 0, "end": 16870, "filename": "/gcode/replicator2/footer.gcode"}, {"audio": 0, "start": 16870, "crunched": 0, "end": 18801, "filename": "/gcode/replicator2x/printer.lua"}, {"audio": 0, "start": 18801, "crunched": 0, "end": 18875, "filename": "/gcode/replicator2x/features.lua"}, {"audio": 0, "start": 18875, "crunched": 0, "end": 19876, "filename": "/gcode/replicator2x/header.gcode"}, {"audio": 0, "start": 19876, "crunched": 0, "end": 20124, "filename": "/gcode/replicator2x/footer.gcode"}, {"audio": 0, "start": 20124, "crunched": 0, "end": 22146, "filename": "/gcode/replicator1_gpx/printer.lua"}, {"audio": 0, "start": 22146, "crunched": 0, "end": 22403, "filename": "/gcode/replicator1_gpx/features.lua"}, {"audio": 0, "start": 22403, "crunched": 0, "end": 23364, "filename": "/gcode/replicator1_gpx/header_1.gcode"}, {"audio": 0, "start": 23364, "crunched": 0, "end": 24410, "filename": "/gcode/replicator1_gpx/header_2.gcode"}, {"audio": 0, "start": 24410, "crunched": 0, "end": 24829, "filename": "/gcode/replicator1_gpx/footer.gcode"}, {"audio": 0, "start": 24829, "crunched": 0, "end": 59272, "filename": "/defaults/settings_reprap.xml"}, {"audio": 0, "start": 59272, "crunched": 0, "end": 93715, "filename": "/defaults/settings_replicator1_gpx.xml"}, {"audio": 0, "start": 93715, "crunched": 0, "end": 128158, "filename": "/defaults/settings_replicator2.xml"}, {"audio": 0, "start": 128158, "crunched": 0, "end": 162601, "filename": "/defaults/settings_replicator2x.xml"}, {"audio": 0, "start": 162601, "crunched": 0, "end": 197044, "filename": "/defaults/settings_ultimaker1.xml"}, {"audio": 0, "start": 197044, "crunched": 0, "end": 231486, "filename": "/defaults/settings_ultimaker2.xml"}, {"audio": 0, "start": 231486, "crunched": 0, "end": 265926, "filename": "/defaults/settings_prusai3.xml"}, {"audio": 0, "start": 265926, "crunched": 0, "end": 266975, "filename": "/defaults/settings_pharaoh_xd.xml"}], "remote_package_size": 266975, "package_uuid": "82e5ca95-695f-456d-8d73-933a1dfce61b"});

})();

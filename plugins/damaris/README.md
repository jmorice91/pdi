# The Damaris plugin {#damaris_plugin}

**WARNING** This documentation is a work in progress and does not reflect the
full potential of this plugin.


**Add description of damaris plugin**
* add Damaris website
  
**Explain the difference between with and without `is_client`**
* [to be modified] the communicator will be split by Damaris in a communicator for the simulation code (client) 
and for damaris execution (client).

**...**

## Configuration grammar

**WARNING** The following grammar will be changed in the next version of this plugin (see https://github.com/jmorice91/pdi/issues/42). 
For this reason, we didn't go into the details. 

* `communicator`: (currently not used) A $-expression referencing an MPI communicator.

* `architecture`: a key-value map.
 * Configuration keys:
    * `sim_name`(string): the name of the simulation  
    * `domains`(integer, default: 1 (??)) : number of blocks by sub domain
    * `dedicated`: a key value map. Configure the division of 
      * Configura keys:
        * `core`: an integer value ($-expresion ??)
        * `node`: an integer value ($-expresion ??)

* `get_is_client`(string): name of `is_client` in PDI data store (optional). It is requested only with `is_client` is used.
* `dc_ending_operations`: #Will be used in the future to provide couple of instructions to execute at the end of the simulation, for the dedicated cores. MayBe relying on user_code plugin!!?
* `client_comm_get` (string):  name of mpi communicator of simulation code (client) in PDI data store (requested). This communicator is defined by Damaris after the split of the communicator defined in `communicator` between client and server.

* `datasets`: list of `DATASET_DESC`
  * `dataset`: a key value map that represent a datset that will be shared with damaris

  * `DATA_DESC` is composed with 
    * `name`(string): name of the dataset
    * `layout`(string): name of the layout of this dataset
    * `storage`(string): name of the storage of this dataset

* `layouts`: list of `LAYOUT_DESC`.
  * `layout:` a key value map that represent a layout defined in a `dataset`.

* `storages`: list of `STORAGE_DESC`.
  * `storage:` a key value map that represent the storage used to save a`dataset`.

* `write`: list of pdi shared with pdi that will be write on the disk by damaris.

* `log`: a key value map that species the logger information of damaris. This feature is optional.
  * Configuration keys:
    * `file_name`(string) The beginning of the log filename. By default is the value of damaris/architecture/sim_name and the default folder is "where_you_launch_the_script/log". The suffix of the filename is `_P4_6.log` where ....
    * `rotation_size` (integer): 5  
    * `log_level`(string): level of the logger. The value are: trace, debug, info, warning, error and fatal
    * `flush`(string): Forces the log file to be flushed if the value is true . The values are true or false.


### comment on this part

**example of different architecture for us**

**question: can we have both core and node in the same simulation?**

**question: how we can test the node case in the CI?**

**Question: what is the other type for a storage ( hdf5, ...)**

* the keyword `when:` can be used in `write` to define the frequency?
* For the layout structure, we can use the same definition as decl_hdf5 plugin to be homogenous  ==> modularity between plugin !!
* In decl_hdf5 plugin in some sense, layout and write can be "fusioné"?

All this points can be adressed in a new version in damaris plugin.

## full configuration example

```yaml
damaris:
  communicator: $MPI_COMM_WORLD # pas utilisé actuellement
  architecture: 
    sim_name: example  
    domains: 1 # => nb de block par sous domain 
    # damaris divise le sous domain par block equitablement
    dedicated:
      core: 1 # number of core in a node dedicated to damaris
      node: 0 # number of node dedicated to damaris
  datasets:
    - dataset: 
        name: main_field
        layout: main_field_layout
        storage: hdf5_example  
        mesh: mesh2d          # pour la visualisation (???)
        centering: zonal      # related to mesh (???)
        script:               # python script ???
        visualizable: true    # pour la visualisation avec paraview (???)
        time_varying: true    # mesh dependent or time in the dataset dimension (???)
        #comment: This is the zonal pressure from our test simulation
  layouts: # on ne peut pas modifier la valuer de layout une fois initialisé, travail en cours pour la modification dynamique
    - layout: 
        name: main_field_layout # ~hdf5 dataset_selection
        type: double            # type of an element of the dataset
        global: '$psize[0]*($dsize[0]-2),$psize[1]*($dsize[1]-2)'
        dimensions: [ '$dsize[0]', '$dsize[1]' ] # process dim, with ghosts/boundaries
        ghosts: '1:1,1:1'   # Number of layers ghost for each dimension. "," separate two dimensions and ":" separate left and right layers in each dimension
        depends_on: [dsize, psize] # This will help ensure an update of the layout attributes value to Damaris lib once the metadata are exposed (using Damaris Parameters in the background)
  storages:
    - storage:
        name: hdf5_example
        type: HDF5
        file_mode: Collective # or FilePerCore
        files_path: ./HDF5_files_damaris_plugin_is_client/   # Where to save files
        # hdf5 file name defined by damaris   
  #Events sections
  write: 
    main_field: # the name of the data to write, if dataset not specified afterward!
      dataset: main_field 
      when: '$iter<10'   # do only write the first 10 iterations (0...9), Default at every iteration.
      position: ['($dsize[0]-2)*$pcoord[0]', '($dsize[1]-2)*$pcoord[1]'] # ~start de dataset_selection par iteration
      # damaris "append" file with next iteration
      #block: [...] # To be defined # indice de block à écrire = 0 par default => tout le sous-domain
      # si "architecture/domains" >1 on peut écrire les block de indice 0 à domains-1
  get_is_client: is_client  # Give name of the variable shares with that correspond to "is_client" 
  dc_ending_operations: #Will be used in the future to provide couple of instructions to execute at the end of the simulation, for the dedicated cores. MayBe relying on user_code plugin!!?
  client_comm_get: mpi_comm # Needed to return the Damaris client comm

  #Optional config, has a default behavior 
  log:
    file_name: example # default: value of damaris/architecture/sim_name and the default folder is "where_you_launch_the_script/log"
    rotation_size: 5
    log_level: info
    flush: true
```

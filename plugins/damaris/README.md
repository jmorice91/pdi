# The Damaris plugin {#damaris_plugin}

**WARNING** This documentation is a work in progress and does not reflect the
full potential of this plugin.


**Add description of damaris plugin**

**Add explication of the difference between with and without `is_client**

**...**

## Configuration grammar

* `communicator`: a $-expression referencing a MPI communicator. This communicator will be split by Damaris in a communicator for the simulation code (client) 
and for damaris execution (client). (not use today)
* `architecture`: a key-value map ...
 * Configuration keys:
    * `sim_name`(string, default: 0) : the total number of subfiles to generate. If set to -1 or other negative values, the one-subfile-per-node mode will be triggered.
    * `domains`(integer, default: 1 (??)) : number of blocks by sub domain
    * `dedicated`: a key value map. Configure the division of 
      * Configura keys:
        * `core`: an integer value ($-expresion ??)
        * `node`: an integer value ($-expresion ??)

* `get_is_client`(string): name of `is_client` in PDI data store (optional). It is requested only with `is_client` is used.
* `dc_ending_operations`: #Will be used in the future to provide couple of instructions to execute at the end of the simulation, for the dedicated cores. MayBe relying on user_code plugin!!?
* `client_comm_get` (string):  name of mpi communicator of simulation code (client) in PDI data store (requested). This communicator is defined by Damaris after the split of the communicator defined in `communicator` between client and server.

* `datasets`: list of `DATASET_DESC`
  * `dataset`: a key value map that represent a  

* `layouts`: list of `LAYOUT_DESC`.
  * `layout:` ...

* `storages`: list of `STORAGE_DESC`.
  * `storage:` ...

* `write`: a key value map ...

* `log`: a key value map that species the logger information of damaris. This feature is optional.
  * Configuration keys:


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
      core: 1
      node: 0 
  datasets:
    - dataset: 
        name: main_field
        layout: main_field_layout
        mesh: mesh2d # pour la visualisation
        centering: zonal
        storage: hdf5_example
        script:
        visualizable: true
        time_varying: true
        #comment: This is the zonal pressure from our test simulation
  layouts: # on ne peut pas modifier la valuer de layout une fois initialisé, travail en cours pour la modification dynamique
    - layout: 
        name: main_field_layout # ~hdf5 dataset_selection
        type: double
        global: '$psize[0]*($dsize[0]-2),$psize[1]*($dsize[1]-2)'
        dimensions: [ '$dsize[0]', '$dsize[1]' ] # process dim, with ghosts/boundaries
        ghosts: '1:1,1:1'   # 1 ghost à gauche de dim1, 1 ghost à droit de dim1 , 1 ghost à gauche de dim2, 1 ghost à droit de dim2
        depends_on: [dsize, psize] # This will help ensure an update of the layout attributes value to Damaris lib once the metadata are exposed (using Damaris Parameters in the background)
  storages:
    - storage:
        name: hdf5_example
        type: HDF5
        file_mode: Collective # or FilePerCore
        files_path: ./HDF5_files_damaris_plugin_is_client/   # Where to save files
        # hdf5 file name defined by damaris
        #frequency: 1    
  #Events sections
  write: 
    main_field: # the name of the data to write, if dataset not specified afterward! ~meme notion de dataset hdf5
      dataset: main_field 
      when: '$iter<10'   # do only write the first 10 iterations (0...9), Default at every iteration.
      #position: ['$pcoord[0]', '$pcoord[1]']
      position: ['($dsize[0]-2)*$pcoord[0]', '($dsize[1]-2)*$pcoord[1]'] # ~start de dataset_selection par iteration
      # damaris "append" file with next iteration
      #block: [...] # To be defined # indice de block à écrire = 0 par default => tout le sous-domain
      # si "architecture/domains" >1 on peut écrire les block de indice 0 à domains-1
  get_is_client: is_client #Needed only when is_client is used
  dc_ending_operations: #Will be used in the future to provide couple of instructions to execute at the end of the simulation, for the dedicated cores. MayBe relying on user_code plugin!!?
  client_comm_get: mpi_comm #Needed to return the Damaris client comm

  #Optional config, has a default behavior 
  log:
    #file_name: example # default = $sim_name
    rotation_size: 5
    log_level: info
    flush: true
```

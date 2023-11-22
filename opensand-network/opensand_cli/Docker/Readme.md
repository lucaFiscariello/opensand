## Esecuzione 
Per lanciare la rete di container che emula una rete satellitare è sufficiente eseguire : 

```c 
 sudo docker compose up build  
```
Per accedere a un container è necessario eseguire :
```c 

 sudo docker exec -it  nome-container  bash
  
```
Alcuni comandi utili per ferificare se la rete è stata creata correttamente:
```c 
ip a # visualizzare le interfacce
tcpdump -ni nome-interfaccia # visualizza il traffico in ingresso e uscita da essa
```

Per vedere se tutto funziona correttamente effettuare ping tra le entità usando gli ip delle interfacce create da opensand. Tali interfaccie hanno come nome *opensand_br*
e un indirizzo ip del tipo 192.168.63.16/24.

## Descrizione xml
Alla creazione di una nuova entità opensand le associa degli xml di deaful che sono tutti uguali indipendentemente dal tipo di entità e 
indipendentemente dal numero di entità già create in precedenza. Questo vuol dire che per ogni entità ci saranno dei campi inutili che non ha senso compilare. 
Di seguito alcuni dei campi più importanti per ogni xml:

### Infrastructure
Nella sezione *<entity>* di Infrastructure.xml viene descritta la singola entità. Opensand impone il vincolo che debbano essere presenti tutti i campi *<entity_sat> ,  <entity_gw> , <entity_gw_net_acc> ..etc*.
Tuttavia l'unico campo che verrà preso in considerazione da opensand quello corrispondente al tipo specificato in *</entity_type>*. Ad esempio se *</entity_type>* è *Gateway* allora si andrà a leggere
solo la sezione *<entity_gw>*, tutte le altre verranno ignorate.


```xml
<entity>
      <entity_type>Gateway</entity_type>      
      <entity_sat>
        <entity_id>-1</entity_id>
        <emu_address/>
        <isl_settings/>
      </entity_sat>
      <entity_gw>
        <entity_id>0</entity_id>
        <emu_address>192.168.0.3</emu_address>
        <tap_iface>opensand_tap</tap_iface>
        <mac_address>00:00:00:00:00:01</mac_address>
      </entity_gw>
      <entity_gw_net_acc>
        <entity_id>-1</entity_id>
        <tap_iface/>
        <mac_address/>
        <interconnect_params>
          <interconnect_address/>
          <interconnect_remote/>
        </interconnect_params>
      </entity_gw_net_acc>
      <entity_gw_phy>
        <entity_id>-1</entity_id>
        <interconnect_params>
          <interconnect_address/>
          <interconnect_remote/>
        </interconnect_params>
        <emu_address/>
      </entity_gw_phy>      
      <entity_st>
        <entity_id>-1</entity_id>
        <emu_address/>
        <tap_iface/>
        <mac_address/>
      </entity_st>
    </entity>
```


Nella sezione * <infrastructure>* sono elencate tutte le entità della rete con le loro caratteristiche. Qui per ogni entità devono essere elencate TUTTE le entità della rete.
Nessun parametro è superfluo. Attenzione a configurare correttamente anche il mac_adress, assicurarsi che sia lo stesso che viene usato nel make network.

```xml
 <infrastructure>
      <satellites>
        <item>
          <entity_id>2</entity_id>
          <emu_address>192.168.0.1</emu_address>
        </item>
      </satellites>
      <gateways>
        <item>
          <entity_id>0</entity_id>
          <emu_address>192.168.0.3</emu_address>
          <mac_address>00:00:00:00:00:01</mac_address>
        </item>
      </gateways>
      <terminals>
        <item>
          <entity_id>1</entity_id>
          <emu_address>192.168.0.2</emu_address>
          <mac_address>00:00:00:00:00:02</mac_address>
        </item>
        <item>
          <entity_id>5</entity_id>
          <emu_address>192.168.0.5</emu_address>
          <mac_address>00:00:00:00:00:05</mac_address>
        </item>
      </terminals>
      <default_gw>0</default_gw>
    </infrastructure>
```

### Topology

La sezione spots contiene la descrizione degli spot. All'interno di ogni *<item>* è sono descritte le configurazioni. La configurazione dello spot associa un gateway a un satellite.

```xml
<spots>
        <item>
          <assignments>
            <gateway_id>0</gateway_id>
            <sat_id_gw>2</sat_id_gw>
            <sat_id_st>2</sat_id_st>
            <forward_regen_level>Transparent</forward_regen_level>
            <return_regen_level>Transparent</return_regen_level>
          </assignments>
        ...
        </item>
</spots>
```

La sezione *<st_assignment>* permette di configurare l'associazione tra un gw e un st. Nell'esempio sottostante ad Gateway con id 0 sono associati due Terminali con id 1 e 5.

```xml
    </st_assignment>
     <defaults>
        <default_gateway>0</default_gateway>
        <default_group>Standard</default_group>
      </defaults>
      <assignments>
        <item>
          <terminal_id>1</terminal_id>
          <gateway_id>0</gateway_id>
          <group>Standard</group>
        </item>
        <item>
          <terminal_id>5</terminal_id>
          <gateway_id>0</gateway_id>
          <group>Standard</group>
        </item>
      </assignments>
    </st_assignment>
```


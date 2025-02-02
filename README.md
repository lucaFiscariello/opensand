OpenSAND is an user-friendly and efficient tool to emulate satellite
communication systems, mainly DVB-RCS - DVB-S2.

It provides a suitable and simple means for performance evaluation and
innovative access and network techniques validation. Its ability to interconnect
real equipments with real applications provides excellent demonstration means.

The source codes of OpenSAND components is distributed "as is" under the terms
and conditions of the GNU GPLv3 license or the GNU LGPLv3 license.

Visit us at [opensand.org](https://www.opensand.org/).

# OpenSAND v6.1.0

## Manuals

 * [Installation Manual](README.md#installation-manual)
 * [Compilation Manual](opensand-packaging/README.md)
 * [Command-line User Manual](opensand-network/opensand_cli/README.md)
 * [Web-interface User Manual](opensand-deploy/README.md)

# Installation Manual

This page describes how to install a simple OpenSAND platform, containing one satellite,
one gateway (GW), and one Satellite terminal (ST).

## Requirements

### Architecture

In order to deploy a platform, a minimum of three machines must be available:

- one for the satellite,
- one for the gateway,
- and one for the satellite terminal.

They must be connected as shown in the image below, with the following networks:

- one network for the emulated satellite link (`EMU`)
- one network for the workstations connected to the GW (`LAN-GW0`)
- one network for the workstations connected to the ST (`LAN-ST1`)

![Installation schema](/schema_install.png)

In the image, two additional machines (`WS-ST1` and `WS-GW0`) are shown but are actually not
necessary; traffic can be exchanged between the ST1 and GW0 without the need of workstations.
However, the interfaces for the networks `LAN-ST1` and `LAN-GW0` must exist, even if no other
computers are connected to it.

### Operating System

The testbed was tested using Ubuntu 20.04 LTS, but it should work on every Linux distribution
or Unix-like system provided that the required dependencies are installed. However, if you
are not on a Debian-based system, you may need to [compile OpenSAND](opensand-packaging/README.md)
yourself.

## Install

This manual describes how to obtain and install OpenSAND using the distributed debian
packages. For more information about how to obtain OpenSAND sources, compile it, and
install it, please refer to the [compilation manual](opensand-packaging/README.md).

The debian packages can be installed using `apt` commands.

Unless otherwise specified, all the commands here must be executed on all machines.

In order to install the packages using `apt`, the repository must be added to its sources. One way of doing it is by creating the file `/etc/apt/sources.list.d/net4sat.list`.

Start by adding the GPG key for the Net4Sat repository:

```
curl -sS https://raw.githubusercontent.com/CNES/net4sat-packages/master/gpg/net4sat.gpg.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/net4sat.gpg >/dev/null
```

Then add the repository and link it to the aforementionned GPG key:

```
echo "deb https://raw.githubusercontent.com/CNES/net4sat-packages/master/jammy/ jammy stable" | sudo tee /etc/apt/sources.list.d/net4sat.list
```

An apt sources update is necessary after adding the repository:

```
sudo apt update
```

Next, the packages must be installed. In this manual, the OpenSAND collector are installed on
the machine with the satellite component, but they can be installed anywhere, even on a machine
without opensand. The only constraint is that all the machines must be connected to a same network.

The OpenSAND core, with all its dependencies, is distributed with the meta-package `opensand`; the
OpenSAND collector with `opensand-collector`; and the OpenSAND configuration tool with `opensand-deploy`.

To install the OpenSAND core on the ST, GW and SAT, execute:

```
sudo apt install opensand
```

### Optional configuration

To install the OpenSAND configuration tool, execute:

```
sudo apt install opensand-deploy
```

The configuration tool is a web server and frontend allowing you to create configuration files
for your OpenSAND entities and remotely deploy them to launch OpenSAND binaries. More information
is available in the [dedicated documentation](opensand-deploy/README.md).

### Optional collector

Before installing the collector, some packages need to be installed first:

- Kibana v6.2.4
- Chronograf v1.7.3
- Logstash v6.2.4
- ElacticSearch v6.2.4
- InfluxDB v1.7.1

Logstash needs to have Java installed. JDK version 8 is recommended for this version of logstash, and can be installed via:

```
sudo apt install openjdk-8-jdk
```

To do so, execute the following commands to download the packages:

```
wget https://artifacts.elastic.co/downloads/kibana/kibana-6.2.4-amd64.deb
wget https://dl.influxdata.com/chronograf/releases/chronograf_1.7.3_amd64.deb
wget https://artifacts.elastic.co/downloads/logstash/logstash-6.2.4.deb
wget https://artifacts.elastic.co/downloads/elasticsearch/elasticsearch-6.2.4.deb
wget https://dl.influxdata.com/influxdb/releases/influxdb_1.7.1_amd64.deb
```

Then install the downloaded packages:

```
sudo dpkg -i kibana-6.2.4-amd64.deb
sudo dpkg -i chronograf_1.7.3_amd64.deb
sudo dpkg -i logstash-6.2.4.deb
sudo dpkg -i elasticsearch-6.2.4.deb
sudo dpkg -i influxdb_1.7.1_amd64.deb
```

To install the OpenSAND collector, execute:

```
sudo apt install opensand-collector
```

> :warning: **If you are using OpenSAND through the OpenBach orchestrator, do **not** install this
collector on the same machine than the OpenBach collector. Even though the tools installed are the
same and used similarly, they have a mutually exclusive configuration. Installing one will break
the other.**

## Uninstall

In order to uninstall OpenSAND, you can remove every OpenSAND package easily by executing:

```
sudo apt remove --purge opensand* libopensand* librle libgse
```

Be careful not to execute the command while on an OpenSAND folder (uninstall may try to remove said folder).

## Deploy 
L'architettura può essere vista come l'insieme di due componenti :
- [un client react](https://github.com/CNES/opensand/tree/ffda4b2e7547cfbfaf1a457a07a38df3d947aedb/opensand-deploy/src/frontend)
- [un server python](https://github.com/CNES/opensand/blob/ffda4b2e7547cfbfaf1a457a07a38df3d947aedb/opensand-deploy/src/backend/backend.py)
  
Il client reat offre l'interfaccia grafica per la configurazione dei vari parametri. La configurazione dei parametri a livello grafico si traduce nella creazione di xml di configurazione che dovranno essere inviati al server. Possono essere creati diverse tipologie di xml:
1. infrastructure
2. topology
3. profile
   
E' possibile creare questi tre file di configurazione per le tre diverse entità:
1. gateway
2. sat
3. st

Il server offre una serie di API per salvare, modificare e eliminare file di configurazioni. Ha inoltre la responsabilità di inoltrare i file di configurazione(tramite ssh)alle macchine virtuali che ospitano le varie entità (gw,sat,st). La comunicazione tra client e server avviene tramite HTTP.

![opensatRange drawio](https://github.com/CNES/opensand/assets/80633764/da1d286e-fa89-4400-b34e-89316c468052)

### Server
Il server prima di inoltrare gli xml alle macchine virtuali li salva in locale in una directory configurabile al momento del lancio del server. Il server riceve dal client gli xml gia pronti, per cui le funzionalità che offre vanno a modificare/aggiungere/eliminare i file mantenuti nella directory di lavoro.
Le api piu importanti offerte dal server sono :
- gestione profile.xml -> **/api/project/<string:name>/profile/<string:entity>**
- gestione topology.xml ->  **/api/project/<string:name>/topology**
- gestione infrastructure.xml -> **/api/project/<string:name>/infrastructure/<string:entity>**

### Client
La creazione e modifica degli xml da parte del client avviene in due fasi :
1. Il client contatta il server e chiede l'xml che ha intenzione di modificare. Tale xml può essere o un template creato dal server in fase di inizializzazione o un xml creato dal client in precedenza.
2. L'utente modifica tramite interfaccia grafica i parametri dell'xml e riinviare al server la nuova versione del file di configurazione.

Si noti come la comunicazione tra client e server avviene tramite scambio di interi xml. E' quindi responsabilità del client generare la versione aggiornata del file e inviarla al server il quale provvederà a scaricare il file e salvarlo in una directory locale prima di inoltrarlo.
In particolare la generazione dei file di configurazione del client avviene tramite vari componenti react. Il flusso di esecuzione è il seguente:. 

- il client [scarica](https://github.com/CNES/opensand/blob/ffda4b2e7547cfbfaf1a457a07a38df3d947aedb/opensand-deploy/src/frontend/src/components/Editor/Editor.tsx#L32C1-L32C64) dal server il file che vuole modificare 
- l'xml è convertito in un oggetto denominato model
- l'oggetto model è dato in ingresso ad un componente denominato [formik](https://github.com/CNES/opensand/blob/ffda4b2e7547cfbfaf1a457a07a38df3d947aedb/opensand-deploy/src/frontend/src/components/Editor/Model.tsx#L68C13-L83C22)
- formik converte il model in un form modificabile tramite interfaccia grafica
- quando l'utente modifica un parametro del form  tramite l'invocazione di callback viene aggiornato il model
- quando è schiacciato il pulsante "save as template" il model è riconvertito in xml e [inviato al server](https://github.com/CNES/opensand/blob/ffda4b2e7547cfbfaf1a457a07a38df3d947aedb/opensand-deploy/src/frontend/src/components/Editor/SaveAsButton.tsx#L33)

La creazione degli xml avviene pertanto in maniera decentralizzata tramite invocazione di callback che vengono gestite da un componente react.

![opensatRange drawio(2)](https://github.com/CNES/opensand/assets/80633764/1c4d2b43-e461-464a-b2b6-85f496cf6d4a)

### Proposta
Il fatto che la generazione dei form venga demandata a un componente controllabile solo tramite interfaccia grafica rende difficile realizzare una configurazione dei parametri in modo agevole. L'dea è quindi quella di dividere l'interfaccia grafica in due sezioni: una basata sul drag and drop (che permette di configurare un sottoinsieme di parametri piu importanti)e un altra che offre una configurazione più dettagliata  basata su opensand. La parte di interfaccia basata su drag and drop deve a sua volta basarsi su un componente *centralizzato* che offre una serie di funzionalità per la generazioni di nuovi xml senza passare per il componente formik. Tuttavia viene comunque mantenuta la possibilità di realizzare configurazioni molto più dettagliate cliccando un bottone che apre l'interfaccia grafica di opensand integrata nella gui finale del progetto.

![osm drawio](https://github.com/CNES/opensand/assets/80633764/9f3d55c2-1500-4904-a17d-7dc38e58e95c)

## Grafana
## Installazione
```c 

sudo apt-get install -y apt-transport-https
sudo apt-get install -y software-properties-common wget
sudo wget -q -O /usr/share/keyrings/grafana.key https://apt.grafana.com/gpg.key
echo "deb [signed-by=/usr/share/keyrings/grafana.key] https://apt.grafana.com stable main" | sudo tee -a /etc/apt/sources.list.d/grafana.list
echo "deb [signed-by=/usr/share/keyrings/grafana.key] https://apt.grafana.com beta main" | sudo tee -a /etc/apt/sources.list.d/grafana.list
sudo apt-get update
sudo apt-get install grafana
  
```

A questo punto puoi visitare la pagina localhost:3000

## Esportazione grafici
Modifica il file /etc/grafana/grafana.ini in modo da attivare l'export della dashboard. Modificare i due campi:
- allow_embedding
  
  ![Schermata del 2023-07-05 09-54-21](https://github.com/lucaFiscariello/opensand/assets/80633764/b3158b80-aa2b-4fc1-b36a-07504bf0d803)

  
- anonymous access
  
  ![Schermata del 2023-07-05 09-54-44](https://github.com/lucaFiscariello/opensand/assets/80633764/81453bb4-0be1-4b1b-ab51-09a4fc375083)

## API Grafana
### Creare l'organizzazione. 
La chiamata all'api restituisce l'id dell'organizzazione.
```c 
  curl -X POST -H "Content-Type: application/json" -d '{"name":"apiorg"}' http://admin:admin@localhost:3000/api/orgs
```

### Cambiare contesto nella nuova organizzazione.
```c 
  curl -X POST http://admin:admin@localhost:3000/api/user/using/<id of new org>
```

### Creazione TokenAPI.
```c 
  curl -X POST -H "Content-Type: application/json" -d '{"name":"apikeycurl", "role": "Admin"}' http://admin:admin@localhost:3000/api/auth/keys
```

### Crazione nuova Dasboard.
Sostituire a '..' il json della dasboard esportato. Viene mostrato un esempio.
```c
  
  curl -X POST -H "Content-Type: application/json" -d '..' http://localhost:3000/api/dashboards/db


  {
    "dashboard": {
    "annotations": {
      "list": [
        {
          "builtIn": 1,
          "datasource": {
            "type": "grafana",
            "uid": "-- Grafana --"
          },
          "enable": true,
          "hide": true,
          "iconColor": "rgba(0, 211, 255, 1)",
          "name": "Annotations & Alerts",
          "type": "dashboard"
        }
      ]
    },
    "editable": true,
    "fiscalYearStartMonth": 0,
    "graphTooltip": 0,
    "id": null,
    "links": [],
    "liveNow": false,
    "panels": [
      {
        "datasource": {
          "type": "grafana",
          "uid": "grafana"
        },
        "fieldConfig": {
          "defaults": {
            "color": {
              "mode": "palette-classic"
            },
            "custom": {
              "axisCenteredZero": false,
              "axisColorMode": "text",
              "axisLabel": "",
              "axisPlacement": "auto",
              "barAlignment": 0,
              "drawStyle": "line",
              "fillOpacity": 0,
              "gradientMode": "none",
              "hideFrom": {
                "legend": false,
                "tooltip": false,
                "viz": false
              },
              "lineInterpolation": "linear",
              "lineWidth": 1,
              "pointSize": 5,
              "scaleDistribution": {
                "type": "linear"
              },
              "showPoints": "auto",
              "spanNulls": false,
              "stacking": {
                "group": "A",
                "mode": "none"
              },
              "thresholdsStyle": {
                "mode": "off"
              }
            },
            "mappings": [],
            "thresholds": {
              "mode": "absolute",
              "steps": [
                {
                  "color": "green",
                  "value": null
                },
                {
                  "color": "red",
                  "value": 80
                }
              ]
            }
          },
          "overrides": []
        },
        "gridPos": {
          "h": 8,
          "w": 12,
          "x": 0,
          "y": 0
        },
        "id": null,
        "options": {
          "legend": {
            "calcs": [],
            "displayMode": "list",
            "placement": "bottom",
            "showLegend": true
          },
          "tooltip": {
            "mode": "single",
            "sort": "none"
          }
        },
        "targets": [
          {
            "datasource": {
              "type": "datasource",
              "uid": "grafana"
            },
            "queryType": "randomWalk",
            "refId": "A"
          }
        ],
        "title": "Panel Title",
        "type": "timeseries"
      }
    ],
    "refresh": "",
    "schemaVersion": 38,
    "style": "dark",
    "tags": [
      "templated"
    ],
    "templating": {
      "list": []
    },
    "time": {
      "from": "now-6h",
      "to": "now"
    },
    "timepicker": {},
    "timezone": "browser",
    "title": "Provaa",
    "uid": null,
    "version": 1,
    "weekStart": ""
  }
  }
```

Per creare una dashboard bisogna prima creare un template a mano tramite l'interfaccia grafica e poi esportare il json del template in modo che dalle API lo si possa ricreare in maniera identica. Per esportare il json andare : 

$$Dasboard -> tastoCondivisione -> Export$$

![Schermata del 2023-07-05 10-06-49](https://github.com/lucaFiscariello/opensand/assets/80633764/9933ccee-7e8d-493c-aea2-096d3950b17b)

## Generazione xml
Per la generazione degli xml è possibile usare in maniera combinata due classi di opensand:
- **opensand/opensand-conf/src/Configuration.cpp** (versione >= 6.0) : genera gli schemi degli xml topology, profile e infrastructure
- **opensand-manager/opensand_manager_core/opensand_xml_parser.py** (versione 5.2) : modifica un xml a partire dal suo schema. E' possibile modificare o aggiungere elementi.

La classe *test.py* nella root del repository mostra come è possibile usare gli schemi e i metodi della classe opensand_xml_parser per modificare gli xml. Attualmente gli xsd generati sono nella directory *opensand-deploy/src/frontend/src/xsd*. Invece la classe di test che sfrutta i metodi implementati da *opensand/opensand-conf/src/Configuration.cpp* è *opensand-core/src/sat_carrier/tests/test.cpp* con l'invocazione del metodo *createModels()*.

# Kypo
Per l'installazione seguire la guida a https://gitlab.ics.muni.cz/muni-kypo-crp/devops/kypo-lite. Per creare una demo seguire la guida https://www.youtube.com/watch?v=9F-m1YDoRtY&t=1003s a partire dal minuto 18.
Alcuni dettagli utili:
- Modifica del file yaml :  git clone -q --bare https://github.com/lucaFiscariello/kypo-opensand.git /repos/lucaFiscariello/kypo-opensand.git
- Aggiunta repository dall'interfaccia:  git@git-internal.kypo:/repos/lucaFiscariello/kypo-opensand.git
- Aggiungi la sandbox da Definition
- Aggiungi Pool
- Lancia Pool








import * as React from 'react';
import { Graph } from "react-d3-graph";
import "./style.css"
import config from "./config";
import data from "./data";
import { useState } from 'react';
import { Button, Stack } from '@mui/material';
import PropTypes from 'prop-types';
import { styled } from '@mui/material/styles';
import Dialog from '@mui/material/Dialog';
import DialogTitle from '@mui/material/DialogTitle';
import DialogContent from '@mui/material/DialogContent';
import DialogActions from '@mui/material/DialogActions';
import IconButton from '@mui/material/IconButton';
import CloseIcon from '@mui/icons-material/Close';
import Typography from '@mui/material/Typography';
import net from './net.png'
import sat from './sat.png'
import swt from './switch.png'
import {useListMutators} from '../../utils/hooks';
import SpacedButton from '../common/SpacedButton';




const BootstrapDialog = styled(Dialog)(({ theme }) => ({
  '& .MuiDialogContent-root': {
    padding: theme.spacing(2),
  },
  '& .MuiDialogActions-root': {
    padding: theme.spacing(1),
  },
}));

function BootstrapDialogTitle(props) {
  const { children, onClose, ...other } = props;

  return (
    <DialogTitle sx={{ m: 0, p: 2 }} {...other}>
      {children}
      {onClose ? (
        <IconButton
          aria-label="close"
          onClick={onClose}
          sx={{
            position: 'absolute',
            right: 8,
            top: 8,
            color: (theme) => theme.palette.grey[500],
          }}
        >
          <CloseIcon />
        </IconButton>
      ) : null}
    </DialogTitle>
  );
}

BootstrapDialogTitle.propTypes = {
  children: PropTypes.node,
  onClose: PropTypes.func.isRequired,
};



export default function Network(props) {

const urlSat = sat
const urlNET = net

const netName = "NET"
const [dataState, setData] = useState({});
const [open, setOpen] = React.useState(false);
const [nodeName, setnodeName] = useState("");
const [tooltip, setTooltip] = useState('');
const [addListItem, removeListItem] = useListMutators(props.list, props.actions, props.form, "elements.0.element.elements.1.element");

React.useEffect(() => {
  const data = {"links":[],"nodes":[]}
  const node = []
  const links = []

  for(let name of props.nameMachines){
    let templateNode = {"id":"","name":"","svg":"","size":400,"labelPosition": 'bottom'}
    let linkTemplate = {"source":netName,"target":name}
  
    templateNode.id = name
    templateNode.name = name
    templateNode.svg = urlSat
  
    node.push(templateNode)
    links.push(linkTemplate)
  }
  
  let templateNode = {"id":"","name":"","svg":"","size":400}
  templateNode.id = netName
  templateNode.name = netName
  templateNode.svg = urlNET
  node.push(templateNode)
  
  
  data.nodes = node
  data.links = links
  setData(data)

}, [props.nameMachines,urlNET,urlSat]);

const handleClickOpen = () => {
  setOpen(true);
};
const handleClose = () => {
  setOpen(false);
};

const handleElimination = () =>{
  const indice = dataState.nodes.findIndex(elemento => elemento.id == nodeName);
  console.log(indice)

  handleClose()
  removeListItem(indice)
}
  
const onDoubleClickNode  = (clickedNodeId) => {
  handleClickOpen()
  setnodeName(clickedNodeId)
};

const onClickNode  = (nodeId) => {
  setTooltip(`Mostra configurazioni del nodo ${nodeId}`);
};


const onClickGraph = function(event) {
  setTooltip('');
};


function AggiungiMacchina() {
      addListItem()
};

  return (

    <Stack>
    <div className="main-container">
        
        <div className='white-box'>
            <Graph id="graph" config={config} data={dataState} onDoubleClickNode={onDoubleClickNode} onClickNode={onClickNode} onClickGraph={onClickGraph} />
        </div>

        {tooltip &&<div className='box-button'>
             <h3>Configurazione</h3>
            <div>
              {tooltip}
              <ul>
                <li>ip</li>
                <li>mac</li>
                <li>altre info</li>
              </ul>
            
            </div>
          </div>}
     
        <BootstrapDialog
          onClose={handleClose}
          aria-labelledby="customized-dialog-title"
          open={open}
        >
        <BootstrapDialogTitle id="customized-dialog-title" onClose={handleClose}>
          Nodo {nodeName}
        </BootstrapDialogTitle>
        <DialogContent dividers>
          <Typography gutterBottom>
            Confermare l'eliminazione del nodo?
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button  onClick={handleElimination}>
            Conferma
          </Button>
        </DialogActions>
      </BootstrapDialog>
      
    </div>
    
    <div className='center-div'>
    <SpacedButton
    color="secondary"
    variant="contained"
    onClick={AggiungiMacchina}
    >
    Add Machine
    </SpacedButton>
    </div>
    </Stack>

  

   
    


    
  );
}
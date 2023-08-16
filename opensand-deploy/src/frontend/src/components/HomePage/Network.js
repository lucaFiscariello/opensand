import * as React from 'react';
import { Graph } from "react-d3-graph";
import "./style.css"
import config from "./config";
import data from "./data";
import { useState } from 'react';
import { Button } from '@mui/material';
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

//const urlSat = "https://img.freepik.com/free-vector/flying-satellite-space-cartoon-icon-illustration_138676-2885.jpg?w=826&t=st=1691603349~exp=1691603949~hmac=b093d1fe8532aa428a81cbf8cd68e5d21a2a8e655e94605466e157e7f09b2bb9"
//const urlNET = "https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcR54J5SezFcHmCgjqlly37Vp-oDfBZ10rbTf3ojDeO2IDDs3R2Yq3Quy0DvXsvSquCNIJ4&usqp=CAU"

const urlSat = sat
const urlGW = sat
const urlST = sat
const urlNET = net
const urlSwt = swt
const node = []
const links = []

const data = {"links":[],"nodes":[]}
const netName = "NET"


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

const [dataState, setData] = useState(data);
const [open, setOpen] = React.useState(false);
const [nodeName, setnodeName] = useState("");
const [tooltip, setTooltip] = useState('');
const [addListItem, removeListItem] = useListMutators(props.list, props.actions, props.form, "elements.0.element.elements.1.element");


const handleClickOpen = () => {
  setOpen(true);
};
const handleClose = () => {
  setOpen(false);
};
  
const onDoubleClickNode  = (clickedNodeId) => {
  handleClickOpen()
  setnodeName(clickedNodeId)
};

const onClickNode  = (clickedNodeId) => {
  handleClickOpen()
  setnodeName(clickedNodeId)
};


const onMouseOverNode = (nodeId, event) => {
  setTooltip(`Mostra configurazioni del nodo ${nodeId}`);
};

const onMouseOutNode = () => {
  setTooltip('');
};


function AggiungiSat() {
      addListItem()
};

function AggiungiGW() {
  let newNode = {"id":"GW1","name":"","svg":urlGW,"size":400,"x":100,"y":100}
  setData({
      ...dataState,
      nodes: [...dataState.nodes, newNode],
    });


};

function AggiungiST() {
  let newNode = {"id":"ST","name":"","svg":urlST,"size":400,"x":100,"y":100}
  setData({
      ...dataState,
      nodes: [...dataState.nodes, newNode],
    });


};


function AggiungiNet() {
  let newNode = {"id":"Net1","name":"","svg":urlNET,"size":400,"x":100,"y":100}
  setData({
      ...dataState,
      nodes: [...dataState.nodes, newNode],
    });


};

function Aggiungiswitch() {
  let newNode = {"id":"Switch","name":"","svg":urlSwt,"size":400,"x":100,"y":100}
  setData({
      ...dataState,
      nodes: [...dataState.nodes, newNode],
    });


};


  return (

    <div className="main-container">
        
        <div className='white-box'>
            <Graph id="graph" config={config} data={dataState} onDoubleClickNode={onDoubleClickNode} onClickNode={onClickNode} onMouseOutNode={onMouseOutNode} onMouseOverNode={onMouseOverNode}/>
        </div>

        <div className='box-button'>
            <div className='space'></div>
            <button className="button" onClick={AggiungiSat}> Aggiungi SAT</button>
            <button className="button" onClick={AggiungiGW}> Aggiungi GW</button>
            <button className="button" onClick={AggiungiST}> Aggiungi ST</button>
            <button className="button" onClick={AggiungiNet}> Aggiungi Rete</button>   
            <button className="button" onClick={Aggiungiswitch}> Aggiungi Switch</button> 
            {tooltip && <div>{tooltip}</div>}

     
        <BootstrapDialog
          onClose={handleClose}
          aria-labelledby="customized-dialog-title"
          open={open}
        >
        <BootstrapDialogTitle id="customized-dialog-title" onClose={handleClose}>
          Modifica configurazioni di {nodeName}
        </BootstrapDialogTitle>
        <DialogContent dividers>
          <Typography gutterBottom>
            Cras mattis consectetur purus sit amet fermentum. Cras justo odio,
            dapibus ac facilisis in, egestas eget quam. Morbi leo risus, porta ac
            consectetur ac, vestibulum at eros.
          </Typography>
          <Typography gutterBottom>
            Praesent commodo cursus magna, vel scelerisque nisl consectetur et.
            Vivamus sagittis lacus vel augue laoreet rutrum faucibus dolor auctor.
          </Typography>
          <Typography gutterBottom>
            Aenean lacinia bibendum nulla sed consectetur. Praesent commodo cursus
            magna, vel scelerisque nisl consectetur et. Donec sed odio dui. Donec
            ullamcorper nulla non metus auctor fringilla.
          </Typography>
        </DialogContent>
        <DialogActions>
          <Button autoFocus onClick={handleClose}>
            Save changes
          </Button>
        </DialogActions>
      </BootstrapDialog>
    
        </div>
      

    </div>
    
  );
}
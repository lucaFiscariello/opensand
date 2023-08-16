import React from 'react';

import SpacedButton from '../common/SpacedButton';
import { useListMutators } from '../../utils/hooks';




const AddEntitiesButton: React.FC<Props> = (props) => {
  
    const [addListItem, removeListItem] = useListMutators(props.list, props.actions, props.form, "elements.0.element.elements.1.element");

    const handleClick = React.useCallback(() => {
        addListItem()
    }, []);


    return (
        <SpacedButton
            color="success"
            variant="contained"
            onClick={handleClick}
        >
            Add Entity
        </SpacedButton>
    );
};


interface Props {
    nameMachines: any,
    form:any,
    list:any,
    actions:any
}


export default AddEntitiesButton;

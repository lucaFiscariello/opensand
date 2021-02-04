import React from 'react';

import FormControl from '@material-ui/core/FormControl';
import Divider from '@material-ui/core/Divider';
import IconButton from '@material-ui/core/IconButton';
import InputLabel from '@material-ui/core/InputLabel';
import ListSubheader from '@material-ui/core/ListSubheader';
import MenuItem from '@material-ui/core/MenuItem';
import Select from '@material-ui/core/Select';
import TextField from '@material-ui/core/TextField';

import DeleteIcon from '@material-ui/icons/HighlightOff';
import EditIcon from '@material-ui/icons/Edit';

import {ITemplatesContent} from '../../api';
import {parameterStyles} from '../../utils/theme';
import {Parameter, Enum} from '../../xsd/model';


interface Props {
    parameter: Parameter;
    templates: ITemplatesContent;
    forceUpdate: () => void;
    onEdit: (model: string, xsd: string, xml?: string) => void;
    enumeration?: Enum;
}


interface Option {
    xsd: string;
    xml?: string;
}


const ProjectParameter = (props: Props) => {
    const {parameter, templates, enumeration, forceUpdate, onEdit} = props;
    const classes = parameterStyles();

    const handleChange = React.useCallback((event: React.ChangeEvent<{name?: string; value: Option;}>) => {
        console.log(event.target);
        const {xsd, xml} = event.target.value;
        parameter.value = xsd;
        forceUpdate();
        onEdit(parameter.id, xsd, xml);
    }, [parameter, forceUpdate, onEdit]);

    const handleClear = React.useCallback(() => {
        parameter.value = "";
        forceUpdate();
    }, [parameter, forceUpdate]);

    const handleEdit = React.useCallback(() => {
        onEdit(parameter.id, parameter.value);
    }, [parameter, onEdit]);

    const header = React.useMemo(() => <em>Please select a {parameter.name}</em>, [parameter]);

    const renderValue = React.useCallback((selected: any) => {
        if (selected == null || selected === "") {
            return header;
        } else {
            return selected;
        }
    }, [header]);

    if (enumeration == null) {
        return (
            <div className={classes.spaced}>
                <TextField
                    variant="outlined"
                    label={parameter.name}
                    value={parameter.value}
                    fullWidth
                    disabled
                />
            </div>
        );
    }

    const templatesNames = enumeration.values.filter((v:string) => v !== "");
    const templateMapping: [string, string[]][] = templatesNames.map((v: string) => (
        [v, [...(templates[v] || [])]]
    ));
    const cumulativeSum = ((sum: number) => (
        ([name, templateNames]: [string, string[]]) => {
            const old = sum;
            sum += templateNames.length + 1;
            return old;
        })
    )(templateMapping.length + 4);
    const offsets = templateMapping.map(cumulativeSum);

    const choiceTemplates = templateMapping.map(([name, templateNames]: [string, string[]], i: number) => {
        const offset = offsets[i];
        const choices = templateNames.map((n: string, j: number) => (
            <MenuItem
                // @ts-ignore [1]
                value={{xsd: name, xml: n}}
                key={offset + j + 1}
            >
                {n}
            </MenuItem>
        ));
        return [<ListSubheader key={offset}>{name}</ListSubheader>, ...choices];
    });
    const choiceModels = templateMapping.map(([name, templateNames]: [string, string[]], i: number) => (
        <MenuItem
            // @ts-ignore [1]
            value={{xsd: name}}
            key={i + 2}
        >
            {name}
        </MenuItem>
    ));
    const dividers = [
        <Divider key={choiceModels.length + 2} />,
        <ListSubheader key={choiceModels.length + 3}>From Template</ListSubheader>,
    ];
    const choices = [
        <MenuItem
            // @ts-ignore [1]
            value={{xsd: ""}}
            key={0}
        >
            {header}
        </MenuItem>,
        <ListSubheader key={1}>Models</ListSubheader>,
    ].concat(choiceModels, dividers, ...choiceTemplates);

    const disabled = parameter.value != null && parameter.value !== "";

    return (
        <div className={classes.spaced}>
            <FormControl className={classes.fullWidth}>
                <InputLabel htmlFor={parameter.id}>
                    {parameter.value === "" ? null : parameter.name}
                </InputLabel>
                <Select
                    value={parameter.value}
                    onChange={handleChange as (event: React.ChangeEvent<{name?: string; value: unknown;}>) => void}
                    inputProps={{id: parameter.id}}
                    displayEmpty
                    renderValue={renderValue}
                    disabled={disabled}
                >
                    {choices}
                </Select>
            </FormControl>
            {disabled && <IconButton onClick={handleEdit}><EditIcon /></IconButton>}
            {disabled && <IconButton onClick={handleClear}><DeleteIcon /></IconButton>}
        </div>
    );
};


export default ProjectParameter;

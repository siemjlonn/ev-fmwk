// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
export declare function EverestLogger(config_path: string, process_name?: string): {
    info(message: any): void;
    debug(message: any): void;
    warning(message: any): void;
    error(message: any): void;
    critical(message: any): void;
};

export type Fulfillment = {
    module_id: string;
    implementation_id: string;
}

export type RequirementContext = {
    module: EverestModule;
    fulfillment: Fulfillment;
  };
  
export type ImplementationContext = {
module: EverestModule;
implementation_id: string;
};

export type CommandHandler = (args: any, resolve: (result: any) => void, reject: (error: any) => void) => void;
export type SubscriptionHandler = (variable: any) => void;
export type MQTTSubscriptionHandler = (message: string) => void;
export type UnsubscriptionCallback = () => void;

export type RuntimeSetup = {
    module_id: string;
    manifest_path: string;
    interfaces_dir: string;
}

export type ConfigSet = { [key: string]: boolean|string|number };

export type ModuleConfig = {
    config_sets: {
        implementations: { [implementation_id: string]: ConfigSet };
        module: ConfigSet;
    }
    connections: {
        [requirement_id: string]: Fulfillment[]
    }
}

export type EverestInterface = {
    variables: string[];
    commands: string[];
}


export type ModuleRequirements = {
    [requirement_id: string]: EverestInterface;
}

export type ModuleImplementations = {
    [implementation_id: string]: EverestInterface;
}

export type ModuleMetadata = {
    authors: string[];
    license: string;
}

export declare class EverestModule {
    constructor(rs: RuntimeSetup);

    say_hello(): ModuleConfig;
    init_done(): void;

    call_command(fulfillment: Fulfillment, command_name: string, arguments: any): any;
    publish_variable(implementation_id: string, variable_name: string, variable: any): void;
    implement_command(implementation_id: string, command_name: string, handler: CommandHandler): void;
    subscribe_variable(fulfillment: Fulfillment, variable_name: string, handler: SubscriptionHandler): UnsubscriptionCallback;

    mqtt_publish(topic: string, message: string): void;
    mqtt_subscribe(topic: string, handler: MQTTSubscriptionHandler): UnsubscriptionCallback;

    metadata: ModuleMetadata;
    implementations: ModuleImplementations;
    requirements: ModuleRequirements;
}

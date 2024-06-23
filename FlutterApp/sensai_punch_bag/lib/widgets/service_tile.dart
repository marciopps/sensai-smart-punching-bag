import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';

import "characteristic_tile.dart";

int iCount = 10;

class ServiceTile extends StatelessWidget {
  final BluetoothService service;
  final List<CharacteristicTile> characteristicTiles;

  const ServiceTile(
      {Key? key, required this.service, required this.characteristicTiles})
      : super(key: key);

  Widget buildUuid(BuildContext context) {
    String uuid =
        iCount.toString(); //''; // 0x${service.uuid.str.toUpperCase()}';
    return Text(uuid, style: TextStyle(fontSize: 13));
  }

  @override
  Widget build(BuildContext context) {
    iCount++;
    return characteristicTiles.isNotEmpty
        ? ExpansionTile(
            title: const Column(
              //mainAxisAlignment: MainAxisAlignment.center,
              //crossAxisAlignment: CrossAxisAlignment.start,
              children: <Widget>[
                Text('',
                    style: TextStyle(
                        color: Colors
                            .red)), //Text('Service', style: TextStyle(color: Colors.blue)),
              ],
            ),
            children: characteristicTiles,
          )
        : ListTile(
            title: const Text(''), // Text('Service'),
            subtitle: buildUuid(context),
          );
  }
}

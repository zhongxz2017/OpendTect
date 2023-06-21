import pytest
import json
import numpy as np
from odbind.survey import Survey
from odbind.seismic2d import Seismic2D

def test_Seismic2D_class():
    f3demo = Survey("F3_Demo_2020")
    data = Seismic2D.names(f3demo)
    assert 'Seis' in data
    data = Seismic2D(f3demo, 'Seis')
    info =  {
                'name': 'Seis',
                'line_count': 11,
                'zunit' : 'ms',
                'comp_count': 1,
                'storage_dtype': 'Float`Signed`4`IEEE`Yes',
            }
    assert data.info() == info
    # feature =   {
    #                 'type': 'FeatureCollection',
    #                 'features': [{
    #                                 'type': 'Feature',
    #                                 'properties': info,
    #                                 'geometry': {
    #                                                 'type': 'Polygon',
    #                                                 'coordinates': [[ ['4.644803', '54.796120'],
    #                                                                   ['4.643676', '54.942126'],
    #                                                                   ['5.014355', '54.942512'],
    #                                                                   ['5.014145', '54.796514'],
    #                                                                   ['4.644803', '54.796120']
    #                                                               ]]
    #                                             }
    #                             }]
    #             }
    # assert json.loads(data.feature()) == feature

    assert Seismic2D(f3demo,'Steering_2D').comp_names == ['Inline dip', 'Line dip']
    assert data.line_names == [ 'SSIS-Grid-Strike4', 'SSIS-Grid-Strike5',
                                'SSIS-Grid-Strike6', 'Well correlation-Line',
                                'SSIS-Grid-Dip1', 'SSIS-Grid-Dip2',
                                'SSIS-Grid-Dip3', 'SSIS-Grid-Dip4',
                                'SSIS-Grid-Strike1', 'SSIS-Grid-Strike2',
                                'SSIS-Grid-Strike3']
    assert data.line_info(['SSIS-Grid-Strike4']) == [{'name': 'SSIS-Grid-Strike4', 'trc_range': [5, 679, 1],'z_range': [0, 1848, 4]}]

